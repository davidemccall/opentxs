/************************************************************
 *
 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  EMAIL:
 *  fellowtraveler@opentransactions.org
 *
 *  WEBSITE:
 *  http://www.opentransactions.org/
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This Source Code Form is subject to the terms of the
 *   Mozilla Public License, v. 2.0. If a copy of the MPL
 *   was not distributed with this file, You can obtain one
 *   at http://mozilla.org/MPL/2.0/.
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will
 *   be useful, but WITHOUT ANY WARRANTY; without even the
 *   implied warranty of MERCHANTABILITY or FITNESS FOR A
 *   PARTICULAR PURPOSE.  See the Mozilla Public License
 *   for more details.
 *
 ************************************************************/

#include "opentxs/core/stdafx.hpp"

#include "opentxs/core/cron/OTCronItem.hpp"

#include "opentxs/api/OT.hpp"
#include "opentxs/api/Wallet.hpp"
#include "opentxs/consensus/ClientContext.hpp"
#include "opentxs/consensus/ServerContext.hpp"
#include "opentxs/core/crypto/OTASCIIArmor.hpp"
#include "opentxs/core/recurring/OTPaymentPlan.hpp"
#include "opentxs/core/script/OTSmartContract.hpp"
#include "opentxs/core/trade/OTTrade.hpp"
#include "opentxs/core/util/Assert.hpp"
#include "opentxs/core/util/Common.hpp"
#include "opentxs/core/util/OTFolders.hpp"
#include "opentxs/core/Account.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/Item.hpp"
#include "opentxs/core/Ledger.hpp"
#include "opentxs/core/Log.hpp"
#include "opentxs/core/NumList.hpp"
#include "opentxs/core/Nym.hpp"
#include "opentxs/core/OTStorage.hpp"
#include "opentxs/core/OTTransaction.hpp"
#include "opentxs/core/String.hpp"
#include "opentxs/core/Types.hpp"

#include <inttypes.h>
#include <irrxml/irrXML.hpp>
#include <cstdint>
#include <deque>
#include <memory>
#include <ostream>
#include <set>

// Base class for OTTrade and OTAgreement and OTPaymentPlan.
// OTCron contains lists of these for regular processing.

// static -- class factory.
//
// I just realized, I don't have to use this only for CronItems.
// If I wanted to, I could put ANY Open-Transactions class in here,
// if there was some need for it, and it would work just fine right here.
// Like if I wanted to have different Token types for different cash
// algorithms. All I have to do is change the return type.
//

namespace opentxs
{

OTCronItem* OTCronItem::NewCronItem(const String& strCronItem)
{
    static char buf[45] = "";

    if (!strCronItem.Exists()) {
        otErr << __FUNCTION__
              << ": Empty string was passed in (returning nullptr.)\n";
        return nullptr;
    }

    String strContract(strCronItem);

    if (!strContract.DecodeIfArmored(false)) {
        otErr << __FUNCTION__ << ": Input string apparently was encoded and "
                                 "then failed decoding. Contents: \n"
              << strCronItem << "\n";
        return nullptr;
    }

    strContract.reset(); // for sgets
    buf[0] = 0;          // probably unnecessary.
    bool bGotLine = strContract.sgets(buf, 40);

    if (!bGotLine) return nullptr;

    String strFirstLine(buf);
    // set the "file" pointer within this string back to index 0.
    strContract.reset();

    // Now I feel pretty safe -- the string I'm examining is within
    // the first 45 characters of the beginning of the contract, and
    // it will NOT contain the escape "- " sequence. From there, if
    // it contains the proper sequence, I will instantiate that type.
    if (!strFirstLine.Exists() || strFirstLine.Contains("- -")) return nullptr;

    // By this point we know already that it's not escaped.
    // BUT it might still be ARMORED!

    std::unique_ptr<OTCronItem> pItem;
    // this string is 35 chars long.
    if (strFirstLine.Contains("-----BEGIN SIGNED PAYMENT PLAN-----")) {
        pItem.reset(new OTPaymentPlan);
    }
    // this string is 28 chars long.
    else if (strFirstLine.Contains("-----BEGIN SIGNED TRADE-----")) {
        pItem.reset(new OTTrade);
    }
    // this string is 36 chars long.
    else if (strFirstLine.Contains("-----BEGIN SIGNED SMARTCONTRACT-----")) {
        pItem.reset(new OTSmartContract);
    }
    else {
        return nullptr;
    }

    // Does the contract successfully load from the string passed in?
    if (pItem->LoadContractFromString(strContract)) {
        return pItem.release();
    }
    return nullptr;
}

OTCronItem* OTCronItem::LoadCronReceipt(const int64_t& lTransactionNum)
{
    String strFilename;
    strFilename.Format("%" PRId64 ".crn", lTransactionNum);

    const char* szFoldername = OTFolders::Cron().Get();
    const char* szFilename = strFilename.Get();

    if (!OTDB::Exists(szFoldername, szFilename)) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": File does not exist: " << szFoldername
              << Log::PathSeparator() << szFilename << "\n";
        return nullptr;
    }

    String strFileContents(OTDB::QueryPlainString(
        szFoldername, szFilename)); // <=== LOADING FROM DATA STORE.

    if (strFileContents.GetLength() < 2) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": Error reading file: " << szFoldername
              << Log::PathSeparator() << szFilename << "\n";
        return nullptr;
    }
    else
        // NOTE: NewCronItem can handle the normal cron item contracts, as well
        // as the OT ARMORED version
        // (It will decode the armor before instantiating the contract.)
        // Therefore there's no need HERE in
        // THIS function to do any decoding...
        //
        return OTCronItem::NewCronItem(strFileContents);
}

// static
OTCronItem* OTCronItem::LoadActiveCronReceipt(
    const int64_t& lTransactionNum,
    const Identifier& notaryID) // Client-side only.
{
    String strFilename, strNotaryID(notaryID);
    strFilename.Format("%" PRId64 ".crn", lTransactionNum);

    const char* szFoldername = OTFolders::Cron().Get();
    const char* szFilename = strFilename.Get();

    if (!OTDB::Exists(szFoldername, strNotaryID.Get(), szFilename)) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": File does not exist: " << szFoldername
              << Log::PathSeparator() << strNotaryID << Log::PathSeparator()
              << szFilename << "\n";
        return nullptr;
    }

    String strFileContents(
        OTDB::QueryPlainString(szFoldername, strNotaryID.Get(),
                               szFilename)); // <=== LOADING FROM DATA STORE.

    if (strFileContents.GetLength() < 2) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": Error reading file: " << szFoldername
              << Log::PathSeparator() << strNotaryID << Log::PathSeparator()
              << szFilename << "\n";
        return nullptr;
    }
    else
        // NOTE: NewCronItem can handle the normal cron item contracts, as well
        // as the OT ARMORED version
        // (It will decode the armor before instantiating the contract.)
        // Therefore there's no need HERE in
        // THIS function to do any decoding...
        //
        return OTCronItem::NewCronItem(strFileContents);
}

// static
// Client-side only.
bool OTCronItem::GetActiveCronTransNums(NumList& output,
                                        const Identifier& nymID,
                                        const Identifier& notaryID)
{
    const char* szFoldername = OTFolders::Cron().Get();

    output.Release();

    // We need to load up the local list of active (recurring) transactions.
    //
    String strListFilename(nymID), strNotaryID(notaryID);
    strListFilename.Concatenate(".lst"); // nymID.lst

    if (OTDB::Exists(szFoldername, strNotaryID.Get(), strListFilename.Get())) {
        // Load up existing list, if it exists.
        //
        String strNumlist(OTDB::QueryPlainString(
            szFoldername, strNotaryID.Get(), strListFilename.Get()));

        if (strNumlist.Exists()) {
            if (false ==
                strNumlist.DecodeIfArmored(false)) // bEscapedIsAllowed=true by
                                                   // default.
            {
                otErr << __FUNCTION__
                      << ": List of recurring transactions; string apparently "
                         "was encoded "
                         "and then failed decoding. Contents: \n" << strNumlist
                      << "\n";
                return false;
            }
            else
                output.Add(strNumlist);
        }
    }

    return true;
}

// static
// Client-side only.
bool OTCronItem::EraseActiveCronReceipt(const int64_t& lTransactionNum,
                                        const Identifier& nymID,
                                        const Identifier& notaryID)
{
    String strFilename, strNotaryID(notaryID);
    strFilename.Format("%" PRId64 ".crn", lTransactionNum);

    const char* szFoldername = OTFolders::Cron().Get();
    const char* szFilename = strFilename.Get();

    // Before we remove the cron item receipt itself, first we need to load up
    // the local list of active (recurring) transactions, and remove the number
    // from that list. Otherwise the GUI will continue thinking the transaction
    // is active in cron.
    //
    String strListFilename(nymID);
    strListFilename.Concatenate(".lst"); // nymID.lst

    if (OTDB::Exists(szFoldername, strNotaryID.Get(), strListFilename.Get())) {
        // Load up existing list, to remove the transaction num from it.
        //
        NumList numlist;

        String strNumlist(OTDB::QueryPlainString(
            szFoldername, strNotaryID.Get(), strListFilename.Get()));

        if (strNumlist.Exists()) {
            if (false ==
                strNumlist.DecodeIfArmored(false)) // bEscapedIsAllowed=true by
                                                   // default.
            {
                otErr << __FUNCTION__
                      << ": List of recurring transactions; string apparently "
                         "was encoded "
                         "and then failed decoding. Contents: \n" << strNumlist
                      << "\n";
            }
            else
                numlist.Add(strNumlist);
        }

        strNumlist.Release();

        if (numlist.Count() > 0) numlist.Remove(lTransactionNum);

        if (0 == numlist.Count()) {
            if (!OTDB::EraseValueByKey(szFoldername, strNotaryID.Get(),
                                       strListFilename.Get())) {
                otOut << "OTCronItem::" << __FUNCTION__
                      << ": FYI, failure erasing recurring IDs file: "
                      << szFoldername << Log::PathSeparator() << strNotaryID
                      << Log::PathSeparator() << strListFilename << "\n";
            }
        }
        else {
            numlist.Output(strNumlist);

            String strFinal;
            OTASCIIArmor ascTemp(strNumlist);

            if (false ==
                ascTemp.WriteArmoredString(
                    strFinal, "ACTIVE CRON ITEMS")) // todo hardcoding
            {
                otErr << "OTCronItem::" << __FUNCTION__
                      << ": Error re-saving recurring IDs (failed writing "
                         "armored string): " << szFoldername
                      << Log::PathSeparator() << strNotaryID
                      << Log::PathSeparator() << strListFilename << "\n";
                return false;
            }
            else {
                bool bSaved = OTDB::StorePlainString(
                    strFinal.Get(), szFoldername, strNotaryID.Get(),
                    strListFilename.Get());

                if (!bSaved) {
                    otErr << "OTCronItem::" << __FUNCTION__
                          << ": Error re-saving recurring IDs: " << szFoldername
                          << Log::PathSeparator() << strNotaryID
                          << Log::PathSeparator() << strListFilename << "\n";
                    return false;
                }
            }
        }
    }

    // Now that the list is updated, let's go ahead and erase the actual cron
    // item itself.
    //
    if (!OTDB::Exists(szFoldername, strNotaryID.Get(), szFilename)) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": File does not exist: " << szFoldername
              << Log::PathSeparator() << strNotaryID << Log::PathSeparator()
              << szFilename << "\n";
        return false;
    }

    if (!OTDB::EraseValueByKey(szFoldername, strNotaryID.Get(), szFilename)) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": Error erasing file: " << szFoldername
              << Log::PathSeparator() << strNotaryID << Log::PathSeparator()
              << szFilename << "\n";
        return false;
    }

    return true;
}

bool OTCronItem::SaveActiveCronReceipt(
    const Identifier& theNymID) // Client-side only.
{
    const int64_t lOpeningNum = GetOpeningNumber(theNymID);

    String strFilename, strNotaryID(GetNotaryID());
    strFilename.Format("%" PRId64 ".crn", lOpeningNum);

    const char* szFoldername = OTFolders::Cron().Get(); // cron
    const char* szFilename = strFilename.Get(); // cron/TRANSACTION_NUM.crn

    if (OTDB::Exists(szFoldername, strNotaryID.Get(), szFilename)) {
        otInfo << "OTCronItem::" << __FUNCTION__
               << ": Cron Record already exists for transaction "
               << GetTransactionNum() << " " << szFoldername
               << Log::PathSeparator() << strNotaryID << Log::PathSeparator()
               << szFilename << ", "
                                "overwriting.\n";
        // NOTE: We could just return here. But what if the record we have is
        // corrupted somehow?
        // Might as well just write it there again, so I let this continue
        // running.
    }
    else // It wasn't there already, so we need to save the number in our
           // local list of trans nums.
    {
        String strListFilename(theNymID);
        strListFilename.Concatenate(".lst"); // nymID.lst
        NumList numlist;

        if (OTDB::Exists(szFoldername, strNotaryID.Get(),
                         strListFilename.Get())) {
            // Load up existing list, to add the new transaction num to it.
            //
            String strNumlist(OTDB::QueryPlainString(
                szFoldername, strNotaryID.Get(), strListFilename.Get()));

            if (strNumlist.Exists()) {
                if (false ==
                    strNumlist.DecodeIfArmored(false)) // bEscapedIsAllowed=true
                                                       // by default.
                {
                    otErr << __FUNCTION__
                          << ": Input string apparently was encoded and then"
                             " failed decoding. Contents: \n" << strNumlist
                          << "\n";
                }
                else
                    numlist.Add(strNumlist);
            }
        }

        numlist.Add(lOpeningNum);

        String strNumlist;

        if (numlist.Output(strNumlist)) {
            String strFinal;
            OTASCIIArmor ascTemp(strNumlist);

            if (false ==
                ascTemp.WriteArmoredString(
                    strFinal, "ACTIVE CRON ITEMS")) // todo hardcoding
            {
                otErr << "OTCronItem::" << __FUNCTION__
                      << ": Error saving recurring IDs (failed writing armored "
                         "string): " << szFoldername << Log::PathSeparator()
                      << strNotaryID << Log::PathSeparator() << strListFilename
                      << "\n";
                return false;
            }

            bool bSaved = OTDB::StorePlainString(strFinal.Get(), szFoldername,
                                                 strNotaryID.Get(),
                                                 strListFilename.Get());

            if (!bSaved) {
                otErr << "OTCronItem::" << __FUNCTION__
                      << ": Error saving recurring IDs: " << szFoldername
                      << Log::PathSeparator() << strNotaryID
                      << Log::PathSeparator() << strListFilename << "\n";
                return false;
            }
        }
    }

    String strFinal;
    OTASCIIArmor ascTemp(m_strRawFile);

    if (false ==
        ascTemp.WriteArmoredString(strFinal, m_strContractType.Get())) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": Error saving file (failed writing armored string): "
              << szFoldername << Log::PathSeparator() << strNotaryID
              << Log::PathSeparator() << szFilename << "\n";
        return false;
    }

    bool bSaved = OTDB::StorePlainString(strFinal.Get(), szFoldername,
                                         strNotaryID.Get(), szFilename);

    if (!bSaved) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": Error saving file: " << szFoldername << Log::PathSeparator()
              << strNotaryID << Log::PathSeparator() << szFilename << "\n";
        return false;
    }

    return bSaved;
}

// When first adding anything to Cron, a copy needs to be saved in a folder
// somewhere.
// (Just for our records.) For example, before I start updating the status on
// any Trade,
// I have already saved the user's original Trade object (from his request) to a
// folder.
// Now I have the freedom to ReleaseSignatures on the Trade and re-sign it with
// the
// server's Nym as it updates over time.  The user cannot challenge the Trade
// because
// the server has the original copy on file and sends it with all receipts.

bool OTCronItem::SaveCronReceipt()
{
    String strFilename;
    strFilename.Format("%" PRId64 ".crn", GetTransactionNum());

    const char* szFoldername = OTFolders::Cron().Get(); // cron
    const char* szFilename = strFilename.Get(); // cron/TRANSACTION_NUM.crn

    if (OTDB::Exists(szFoldername, szFilename)) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": Cron Record already exists for transaction "
              << GetTransactionNum() << " " << szFoldername
              << Log::PathSeparator() << szFilename
              << ",\n"
                 "yet inexplicably attempted to record it again.\n";
        return false;
    }

    String strFinal;
    OTASCIIArmor ascTemp(m_strRawFile);

    if (false ==
        ascTemp.WriteArmoredString(strFinal, m_strContractType.Get())) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": Error saving file (failed writing armored string): "
              << szFoldername << Log::PathSeparator() << szFilename << "\n";
        return false;
    }

    bool bSaved =
        OTDB::StorePlainString(strFinal.Get(), szFoldername, szFilename);

    if (!bSaved) {
        otErr << "OTCronItem::" << __FUNCTION__
              << ": Error saving file: " << szFoldername << Log::PathSeparator()
              << szFilename << "\n";
        return false;
    }

    return bSaved;
}

bool OTCronItem::SetDateRange(time64_t VALID_FROM, time64_t VALID_TO)
{
    // Set the CREATION DATE
    //
    const time64_t CURRENT_TIME = OTTimeGetCurrentTime();

    // Set the Creation Date.
    SetCreationDate(CURRENT_TIME);

    // VALID_FROM
    //
    // The default "valid from" time is NOW.
    if (OT_TIME_ZERO >= VALID_FROM) // if it's 0 or less, set to current time.
        SetValidFrom(CURRENT_TIME);
    else // Otherwise use whatever was passed in.
        SetValidFrom(VALID_FROM);

    // VALID_TO
    //
    // The default "valid to" time is 0 (which means no expiration date / cancel
    // anytime.)
    if (OT_TIME_ZERO == VALID_TO) // VALID_TO is 0
    {
        SetValidTo(VALID_TO); // Keep it at zero then, so it won't expire.
    }
    else if (OT_TIME_ZERO < VALID_TO) // VALID_TO is ABOVE zero...
    {
        if (VALID_TO <
            VALID_FROM) // If Valid-To date is EARLIER than Valid-From date...
        {
            int64_t lValidTo = OTTimeGetSecondsFromTime(VALID_TO);
            int64_t lValidFrom = OTTimeGetSecondsFromTime(VALID_FROM);
            otErr << "OTCronItem::" << __FUNCTION__ << ": VALID_TO ("
                  << lValidTo << ") is earlier than VALID_FROM (" << lValidFrom
                  << ")\n";
            return false;
        }

        SetValidTo(VALID_TO); // Set it to whatever it is, since it is now
                              // validated as higher than Valid-From.
    }
    else                    // VALID_TO is a NEGATIVE number... Error.
    {
        int64_t lValidTo = OTTimeGetSecondsFromTime(VALID_TO);
        otErr << "OTCronItem::" << __FUNCTION__
              << ": Negative value for valid_to: " << lValidTo << "\n";

        return false;
    }

    return true;
}

// These are for finalReceipt
// The Cron Item stores a list of these closing transaction numbers,
// used for closing a transaction.
//
int32_t OTCronItem::GetCountClosingNumbers() const
{
    return static_cast<int32_t>(m_dequeClosingNumbers.size());
}

int64_t OTCronItem::GetClosingTransactionNoAt(uint32_t nIndex) const
{
    if (m_dequeClosingNumbers.size() <= nIndex) {
        otErr << __FUNCTION__ << ": "
              << "nIndex"
              << " is equal or larger than m_dequeClosingNumbers.size()!\n";
        OT_FAIL;
    }

    return m_dequeClosingNumbers.at(nIndex);
}

void OTCronItem::AddClosingTransactionNo(const int64_t& lClosingTransactionNo)
{
    m_dequeClosingNumbers.push_back(lClosingTransactionNo);
}

/// See if theNym has rights to remove this item from Cron.
bool OTCronItem::CanRemoveItemFromCron(const ClientContext& context)
{
    const String strNotaryID(GetNotaryID());

    // You don't just go willy-nilly and remove a cron item from a market unless
    // you check first and make sure the Nym who requested it actually has said
    // number (or a related closing number) signed out to him on his last
    // receipt...
    if (!context.Nym()->CompareID(GetSenderNymID())) {
        otLog5 << "OTCronItem::CanRemoveItem: theNym is not the originator of "
               << "this CronItem. (He could be a recipient though, so this is "
               << " normal.)\n";

        return false;
    }
    // By this point, that means theNym is DEFINITELY the originator (sender)...
    else if (GetCountClosingNumbers() < 1) {
        otOut << "Weird: Sender tried to remove a cron item; expected at least "
                 "1 closing number to be available"
                 "--that wasn't. (Found " << GetCountClosingNumbers() << ").\n";

        return false;
    }

    if (!context.VerifyIssuedNumber(GetClosingNum())) {
        otOut << "OTCronItem::CanRemoveItemFromCron: Closing number didn't "
                 "verify (for removal from cron).\n";

        return false;
    }

    // By this point, we KNOW theNym is the sender, and we KNOW there are the
    // proper number of transaction numbers available to close. We also know
    // that this cron item really was on the cron object, since that is where it
    // was looked up from, when this function got called! So I'm pretty sure, at
    // this point, to authorize removal, as long as the transaction num is still
    // issued to theNym (this check here.)

    return context.VerifyIssuedNumber(GetOpeningNum());

    // Normally this will be all we need to check. The originator will have the
    // transaction number signed-out to him still, if he is trying to close it.
    // BUT--in some cases, someone who is NOT the originator can cancel. Like in
    // a payment plan, the sender is also the depositor, who would normally be
    // the person cancelling the plan. But technically, the RECIPIENT should
    // also have the ability to cancel that payment plan.  BUT: the transaction
    // number isn't signed out to the RECIPIENT... In THAT case, the below
    // VerifyIssuedNum() won't work! In those cases, expect that the special
    // code will be in the subclasses override of this function.
    // (OTPaymentPlan::CanRemoveItem() etc)

    // P.S. If you override this function, maybe call the parent
    // (OTCronItem::CanRemoveItem) first, for the VerifyIssuedNum call above.
    // Only if that fails, do you need to dig deeper...
}

// OTCron calls this regularly, which is my chance to expire, etc.
// Child classes will override this, AND call it (to verify valid date range.)
//
// Return False:    REMOVE this Cron Item from Cron.
// Return True:        KEEP this Cron Item on Cron (for now.)
//
bool OTCronItem::ProcessCron()
{
    OT_ASSERT(nullptr != m_pCron);

    if (IsFlaggedForRemoval()) {
        otLog3 << "Cron: Flagged for removal: " << m_strContractType << ".\n";
        return false;
    }

    // I call IsExpired() here instead of VerifyCurrentDate(). The Cron Item
    // will stay on
    // Cron even if it is NOT YET valid. But once it actually expires, this will
    // remove it.
    if (IsExpired()) {
        otLog3 << "Cron: Expired " << m_strContractType << ".\n";
        return false;
    }

    // As far as this code is concerned, the item can stay on cron for now.
    // Return true.
    return true;
}

// OTCron calls this when a cron item is added.
// bForTheFirstTime=true means that this cron item is being
// activated for the very first time. (Versus being re-added
// to cron after a server reboot.)
//
void OTCronItem::HookActivationOnCron(Nym*, // sometimes nullptr.
                                      bool bForTheFirstTime)
{
    // Put anything else in here, that needs to be done in the
    // cron item base class, upon activation. (This executes
    // no matter what, even if onActivate() is overridden.)

    if (bForTheFirstTime) onActivate(); // Subclasses may override this.
                                        //
                                        // MOST NOTABLY,
    // OTSmartContract overrides this, so it can allow the SCRIPT
    // a chance to hook onActivate() as well.
}

// OTCron calls this when a cron item is removed
// This gives each item a chance to drop a final receipt,
// and clean up any memory, before being destroyed.
//
void OTCronItem::HookRemovalFromCron(Nym* pRemover, int64_t newTransactionNo)
{
    Nym* pServerNym = serverNym_;
    OT_ASSERT(nullptr != pServerNym);

    // Generate new transaction number for these new inbox receipts.
    //
    const int64_t lNewTransactionNumber = newTransactionNo;

    //    OT_ASSERT(lNewTransactionNumber > 0); // this can be my reminder.
    if (0 == lNewTransactionNumber) {
        otErr << "OTCronItem::HookRemovalFromCron: ** ERROR Final receipt not "
                 "added to inbox, since no "
                 "transaction numbers were available!\n";
    }
    else {
        // Everytime a payment processes, or a trade, then a receipt is put in
        // the user's inbox.
        // This contains a copy of the current payment or trade (which took
        // money from the user's acct.)
        //
        // ==> So I increment the payment count each time before dropping the
        // receipt. (I also use a fresh
        // transaction number when I put it into the inbox.) That way, the user
        // will never get the same
        // receipt for the same plan twice. It cannot take funds from his
        // account, without a new payment
        // count and a new transaction number on a new receipt. Until the user
        // accepts the receipt out
        // of his inbox with a new balance agreement, the existing receipts can
        // be added up and compared
        // to the last balance agreement, to verify the current balance. Every
        // receipt from a processing
        // payment will have the user's authorization, signature, and terms, as
        // well as the update in balances
        // due to the payment, signed by the server.

        // In the case of the FINAL RECEIPT, I do NOT increment the count, so
        // you can see it will have the same
        // payment count as the last paymentReceipt. (if there were 5
        // paymentReceipts, from 1 to 5, then the
        // finalReceipt will also be 5. This is evidence of what the last
        // paymentReceipt WAS.)

        // The TRANSACTION will be dropped into the INBOX with "In Reference To"
        // information,
        // containing the ORIGINAL SIGNED REQUEST.
        //
        OTCronItem* pOrigCronItem =
            OTCronItem::LoadCronReceipt(GetTransactionNum());
        // OTCronItem::LoadCronReceipt loads the original version with the
        // user's signature.
        // (Updated versions, as processing occurs, are signed by the server.)
        OT_ASSERT(nullptr != pOrigCronItem);
        std::unique_ptr<OTCronItem> theCronItemAngel(pOrigCronItem);

        // Note: elsewhere, we verify the Nym's signature. But in this place, we
        // verify the SERVER's
        // signature. (The server signed the cron receipt just before it was
        // first saved, so it has two signatures on it.)
        //
        {
            bool bValidSignture = pOrigCronItem->VerifySignature(*pServerNym);
            if (!bValidSignture) {
                otErr << __FUNCTION__ << ": Failure verifying signature of "
                                         "server on Cron Item!\n";
                OT_FAIL;
                return;
            }
        }

        // I now have a String copy of the original CronItem...
        const String strOrigCronItem(*pOrigCronItem);

        Nym theOriginatorNym; // Don't use this... use the pointer just
                              // below.

        // The Nym who is actively requesting to remove a cron item will be
        // passed in as pRemover.
        // However, sometimes there is no Nym... perhaps it just expired and
        // pRemover is nullptr.
        // The originating Nym (if different than remover) is loaded up.
        // Otherwise the originator
        // pointer just pointers to *pRemover.
        //
        Nym* pOriginator = nullptr;

        if (pServerNym->CompareID(pOrigCronItem->GetSenderNymID())) {
            pOriginator = pServerNym; // Just in case the originator Nym is also
                                      // the server Nym.
        } // This MIGHT be unnecessary, since pRemover is(I think) already
          // transmogrified
        // ******************************************************* to pServer
        // earlier, if they share the same ID.
        //
        // If pRemover is NOT nullptr, and he has the Originator's ID...
        // then set the pointer accordingly.
        //
        else if ((nullptr != pRemover) &&
                 (true ==
                  pRemover->CompareID(pOrigCronItem->GetSenderNymID()))) {
            pOriginator = pRemover; // <======== now both pointers are set (to
                                    // same Nym). DONE!
        }

        // At this point, pRemover MIGHT be set, or nullptr. (And that's that --
        // pRemover may always be nullptr.)
        //
        // if pRemover IS set, then pOriginator MIGHT point to it as well. (If
        // the IDs match. Done above.)
        // pOriginator might also still be nullptr. (If pRemover is nullptr,
        // then
        // pOriginator DEFINITELY is.)
        // pRemover is loaded (or not). Next let's make SURE pOriginator is
        // loaded, if it wasn't already...
        //
        if (nullptr == pOriginator) {
            // GetSenderNymID() should be the same on THIS (updated version of
            // the same cron item)
            // but for whatever reason, I'm checking the nymID on the original
            // version. Sue me.
            //
            const Identifier NYM_ID(pOrigCronItem->GetSenderNymID());

            theOriginatorNym.SetIdentifier(NYM_ID);

            if (!theOriginatorNym.LoadPublicKey()) {
                String strNymID(NYM_ID);
                otErr << "OTCronItem::HookRemovalFromCron: Failure loading "
                         "Sender's public key:\n" << strNymID << "\n";
            }
            else if (theOriginatorNym.VerifyPseudonym() &&
                       theOriginatorNym.LoadSignedNymfile(
                           *pServerNym)) // ServerNym here is merely the signer
                                         // on this file.
            {
                pOriginator = &theOriginatorNym; //  <=====
            }
            else {
                String strNymID(NYM_ID);
                otErr << "OTCronItem::HookRemovalFromCron: Failure verifying "
                         "Sender's"
                         " public key or loading signed nymfile: " << strNymID
                      << "\n";
            }
        }

        // pOriginator should NEVER be nullptr by this point, unless there was
        // an
        // ERROR in the above block.
        // We even loaded the guy from storage, if we had to.
        //
        if (nullptr != pOriginator) {
            // Drop the FINAL RECEIPT(s) into the user's inbox(es)!!
            // Pass in strOrigCronItem and lNewTransactionNumber which were
            // obtained above.
            //
            onFinalReceipt(*pOrigCronItem, lNewTransactionNumber, *pOriginator,
                           pRemover);
        }
        else {
            otErr << "MAJOR ERROR in OTCronItem::HookRemovalFromCron!! Failed "
                     "loading Originator Nym for Cron Item.\n";
        }
    }

    // Remove corresponding offer from market, if applicable.
    //
    onRemovalFromCron();
}

// This function is overridden in OTTrade, OTAgreement, and OTSmartContract.
//
// I'm put a default implementation here "Just Because."
//
// This is called by HookRemovalFromCron().
//
void OTCronItem::onFinalReceipt(OTCronItem& theOrigCronItem,
                                const int64_t& lNewTransactionNumber,
                                Nym& theOriginator,
                                Nym* pRemover) // may already point to
                                               // theOriginator... or
                                               // someone else...
{
    OT_ASSERT(nullptr != serverNym_);

    Nym& pServerNym = *serverNym_;

    auto context = OT::App().Contract().mutable_ClientContext(
        pServerNym.ID(), theOriginator.ID());

    // The finalReceipt Item's ATTACHMENT contains the UPDATED Cron Item.
    // (With the SERVER's signature on it!)
    //
    String strUpdatedCronItem(*this);
    String* pstrAttachment = &strUpdatedCronItem;

    const String strOrigCronItem(theOrigCronItem);

    // First, we are closing the transaction number ITSELF, of this cron item,
    // as an active issued number on the originating nym. (Changing it to
    // CLOSED.)
    //
    // Second, we're verifying the CLOSING number, and using it as the closing
    // number on the FINAL RECEIPT (with that receipt being "InReferenceTo"
    // GetTransactionNum())
    const TransactionNumber lOpeningNumber = theOrigCronItem.GetOpeningNum();
    const TransactionNumber lClosingNumber = theOrigCronItem.GetClosingNum();
    const String strNotaryID(GetNotaryID());
    // unused unless it's really not already loaded. (use pActualNym.)
    Nym theActualNym;

    // I'm ASSUMING here that pRemover is also theOriginator.
    //
    // REMEMBER: Most subclasses will override this method, and THEY
    // are the cases where pRemover is someone other than theOriginator.
    // That's why they have a different version of onFinalReceipt.
    if ((lOpeningNumber > 0) && context.It().VerifyIssuedNumber(lOpeningNumber))
    {
        // The Nym (server side) stores a list of all opening and closing cron
        // #s. So when the number is released from the Nym, we also take it off
        // that list.
        context.It().CloseCronItem(lOpeningNumber);
        context.It().ConsumeIssued(lOpeningNumber);
        theOriginator.SaveSignedNymfile(pServerNym);

        // the RemoveIssued call means the original transaction# (to find this
        // cron item on cron) is now CLOSED.
        // But the Transaction itself is still OPEN. How? Because the CLOSING
        // number is still signed out.
        // The closing number is also USED, since the NotarizePaymentPlan or
        // NotarizeMarketOffer call, but it
        // remains ISSUED, until the final receipt itself is accepted during a
        // process inbox.
        //
        const Identifier& ACTUAL_NYM_ID = GetSenderNymID();
        Nym* pActualNym = nullptr; // use this. DON'T use theActualNym.

        if (pServerNym.CompareID(ACTUAL_NYM_ID))
            pActualNym = &pServerNym;
        else if (theOriginator.CompareID(ACTUAL_NYM_ID))
            pActualNym = &theOriginator;
        else if ((nullptr != pRemover) && pRemover->CompareID(ACTUAL_NYM_ID))
            pActualNym = pRemover;

        else // We couldn't find the Nym among those already loaded--so we have
             // to load
        {    // it ourselves (so we can update its NymboxHash value.)
            theActualNym.SetIdentifier(ACTUAL_NYM_ID);

            if (!theActualNym.LoadPublicKey()) // Note: this step may be
                                               // unnecessary since we
                                               // are only updating his
                                               // Nymfile, not his key.
            {
                String strNymID(ACTUAL_NYM_ID);
                otErr << __FUNCTION__
                      << ": Failure loading public key for Nym: " << strNymID
                      << ". (To update his NymboxHash.) \n";
            }
            else if (theActualNym.VerifyPseudonym() && // this line may be
                                                         // unnecessary.
                       theActualNym.LoadSignedNymfile(
                           pServerNym)) // ServerNym here is not theActualNym's
                                         // identity, but merely the signer on
                                         // this file.
            {
                otLog3
                    << __FUNCTION__
                    << ": Loading actual Nym, since he wasn't already loaded. "
                       "(To update his NymboxHash.)\n";
                pActualNym = &theActualNym; //  <=====
            }
            else {
                String strNymID(ACTUAL_NYM_ID);
                otErr
                    << __FUNCTION__
                    << ": Failure loading or verifying Actual Nym public key: "
                    << strNymID << ". (To update his NymboxHash.)\n";
            }
        }

        if (!DropFinalReceiptToNymbox(GetSenderNymID(), lNewTransactionNumber,
                                      strOrigCronItem, GetOriginType(),
                                      nullptr, // note
                                      pstrAttachment, pActualNym)) {
            otErr << __FUNCTION__
                  << ": Failure dropping finalReceipt to Nymbox.\n";
        }
    }
    else {
        otErr << __FUNCTION__ << ": Failed doing "
                                 "VerifyIssuedNum(theOrigCronItem."
                                 "GetTransactionNum())\n";
    }

    if ((lClosingNumber > 0) && context.It().VerifyIssuedNumber(lClosingNumber))
    {
        // SENDER only. (CronItem has no recipient. That's in the subclass.)
        if (!DropFinalReceiptToInbox(
                GetSenderNymID(), GetSenderAcctID(), lNewTransactionNumber,
                lClosingNumber,           // The closing transaction number to
                                          // put on the receipt.
                strOrigCronItem, GetOriginType(),
                nullptr, // note
                pstrAttachment))          // pActualAcct = nullptr by default.
                                          // (This call will load it up in order
                                          // to update the inbox hash.)
            otErr << __FUNCTION__ << ": Failure dropping receipt into inbox.\n";

        // In this case, I'm passing nullptr for pstrNote, since there is no
        // note.
        // (Additional information would normally be stored in the note.)

        // This part below doesn't happen until you ACCEPT the final receipt
        // (when processing your inbox.)
        //
        //      theOriginator.RemoveIssuedNum(strNotaryID, lClosingNumber,
        // true); //bSave=false
    } else {
        otErr << __FUNCTION__
              << ": Failed verifying "
                 "lClosingNumber=theOrigCronItem.GetClosingTransactionNoAt(0)>"
                 "0 &&  "
                 "theOriginator.VerifyTransactionNum(lClosingNumber)\n";
    }

    // QUESTION: Won't there be Cron Items that have no asset account at all?
    // In which case, there'd be no need to drop a final receipt, but I don't
    // think
    // that's the case, since you have to use a transaction number to get onto
    // cron
    // in the first place.
}

// This is the "DROPS FINAL RECEIPT" function.
// "Final Receipts" are used by Cron Items, as the last receipt for a given
// transaction number.
//
bool OTCronItem::DropFinalReceiptToInbox(
    const Identifier& NYM_ID, const Identifier& ACCOUNT_ID,
    const int64_t& lNewTransactionNumber, const int64_t& lClosingNumber,
    const String& strOrigCronItem,
    const originType theOriginType,
    String* pstrNote, String* pstrAttachment,
    Account* pActualAcct)
{
    OT_ASSERT(nullptr != serverNym_);

    Nym& pServerNym = *serverNym_;
    const char* szFunc = "OTCronItem::DropFinalReceiptToInbox";
    std::unique_ptr<Account> theDestAcctGuardian;

    // Load the inbox in case it already exists.
    Ledger theInbox(NYM_ID, ACCOUNT_ID, GetNotaryID());

    // Inbox will receive notification of something ALREADY DONE.
    bool bSuccessLoading = theInbox.LoadInbox();

    // ...or generate it otherwise...

    if (true == bSuccessLoading)
        bSuccessLoading = theInbox.VerifyAccount(pServerNym);
    else
        otErr << szFunc << ": ERROR loading inbox ledger.\n";
//      otErr << szFunc << ": ERROR loading inbox ledger.\n";
//  else
//      bSuccessLoading = theInbox.GenerateLedger(ACCOUNT_ID, GetNotaryID(), OTLedger::inbox, true); // bGenerateFile=true

    if (!bSuccessLoading) {
        otErr << szFunc << ": ERROR loading or generating an inbox. (FAILED "
                           "WRITING RECEIPT!!) \n";
        return false;
    }
    else {
        // Start generating the receipts

        OTTransaction* pTrans1 = OTTransaction::GenerateTransaction(
            theInbox, OTTransaction::finalReceipt, theOriginType, lNewTransactionNumber);
        // (No need to OT_ASSERT on the above transaction since it occurs in
        // GenerateTransaction().)

        // The inbox will get a receipt with the new transaction ID.
        // That receipt has an "in reference to" field containing the original
        // cron item.

        OT_ASSERT(nullptr != pTrans1);

        // set up the transaction items (each transaction may have multiple
        // items... but not in this case.)
        Item* pItem1 =
            Item::CreateItemFromTransaction(*pTrans1, Item::finalReceipt);

        // This may be unnecessary, I'll have to check
        // CreateItemFromTransaction. I'll leave it for now.
        OT_ASSERT(nullptr != pItem1);

        pItem1->SetStatus(Item::acknowledgement);

        //
        // Here I make sure that the receipt (the inbox notice) references the
        // transaction number that the trader originally used to issue the cron
        // item...
        // This number is used to match up offers to trades, and used to track
        // all cron items.
        // (All Cron items require a transaction from the user to add them to
        // Cron in the
        // first place.)
        //
        const int64_t lOpeningNum = GetOpeningNumber(NYM_ID);

        pTrans1->SetReferenceToNum(lOpeningNum);
        pTrans1->SetNumberOfOrigin(lOpeningNum);
//      pItem1-> SetReferenceToNum(lOpeningNum);

        // The reference on the transaction contains an OTCronItem, in this
        // case.
        // The original cron item, versus the updated cron item (which is stored
        // on the finalReceipt item just below here.)
        //
        pTrans1->SetReferenceString(strOrigCronItem);

        pTrans1->SetClosingNum(lClosingNumber); // This transaction is the
                                                // finalReceipt for
                                                // GetTransactionNum(), as
                                                // lClosingNumber.
//      pItem1-> SetClosingNum(lClosingNumber);
        //
        // NOTE: I COULD look up the closing number by doing a call to
        // GetClosingNumber(ACCOUNT_ID);
        // But that is already taken care of where it matters, and passed in
        // here properly already, so it
        // would be superfluous.

        // The finalReceipt ITEM's NOTE contains the UPDATED CRON ITEM.
        //
        if (nullptr != pstrNote) {
            pItem1->SetNote(*pstrNote); // in markets, this is updated trade.
        }

        // Also set the ** UPDATED OFFER ** as the ATTACHMENT on the ** item.**
        // (With the SERVER's signature on it!) // in markets, this is updated
        // offer.
        //
        if (nullptr != pstrAttachment) {
            pItem1->SetAttachment(*pstrAttachment);
        }

        // sign the item

        pItem1->SignContract(pServerNym);
        pItem1->SaveContract();

        // the Transaction "owns" the item now and will handle cleaning it up.
        pTrans1->AddItem(*pItem1);

        pTrans1->SignContract(pServerNym);
        pTrans1->SaveContract();

        // Here the transaction we just created is actually added to the ledger.
        theInbox.AddTransaction(*pTrans1);

        // Release any signatures that were there before (They won't
        // verify anymore anyway, since the content has changed.)
        theInbox.ReleaseSignatures();

        // Sign and save.
        theInbox.SignContract(pServerNym);
        theInbox.SaveContract();

        // TODO: Better rollback capabilities in case of failures here:

        if (nullptr == pActualAcct) // no asset account was passed in as already
                                    // loaded, so let's load it ourselves then.
        {
            pActualAcct =
                Account::LoadExistingAccount(ACCOUNT_ID, GetNotaryID());
            theDestAcctGuardian.reset(pActualAcct);
        }

        // Save inbox to storage. (File, DB, wherever it goes.)
        //
        if (nullptr != pActualAcct) {
            OT_ASSERT(ACCOUNT_ID == pActualAcct->GetPurportedAccountID());

            if (pActualAcct->VerifyAccount(pServerNym)) {
                pActualAcct->SaveInbox(theInbox);
                pActualAcct->SaveAccount(); // inbox hash has changed here, so
                                            // we save the account to reflect
                                            // that change.
            }
            else {
                otErr << szFunc
                      << ": Failed: pActualAcct->VerifyAccount(*pServerNym)\n";
            }
        }
        else // todo: would the account EVER be null here? Should never be.
               // Therefore should we save the inbox here?
        {
            theInbox.SaveInbox();
        }

        // Notice above, if the account loads but fails to verify, then we do
        // not save the Inbox.
        // Todo: ponder wisdom of that decision.

        // Corresponds to the AddTransaction() just above.
        // Details are stored in separate file these days.
        //
        pTrans1->SaveBoxReceipt(theInbox);

        return true; // Really this true should be predicated on ALL the above
                     // functions returning true. Right?
    }                // ...Right?
}

// Done: IF ACTUAL NYM is NOT passed below, then need to LOAD HIM UP (so we can
// update his NymboxHash after we update the Nymbox.)

// The final receipt doesn't have a closing number in the Nymbox, only in the
// Inbox.
// That's because in the Nymbox, it's just a notice, and it's not there to
// enforce anything.
// If you get one in your Nymbox, it's just so that you know to remove its "in
// ref to" number (the opening number)
// from your issued list (so your balance agreements will work :P)
//
bool OTCronItem::DropFinalReceiptToNymbox(
    const Identifier& NYM_ID,
    const TransactionNumber& lNewTransactionNumber,
    const String& strOrigCronItem,
    const originType theOriginType,
    String* pstrNote,
    String* pstrAttachment,
    const Nym* pActualNym)
{
    OT_ASSERT(nullptr != serverNym_);

    Nym& pServerNym = *serverNym_;
    const char* szFunc =
        "OTCronItem::DropFinalReceiptToNymbox"; // RESUME!!!!!!!

    Ledger theLedger(NYM_ID, NYM_ID, GetNotaryID());

    // Inbox will receive notification of something ALREADY DONE.
    bool bSuccessLoading = theLedger.LoadNymbox();

    // ...or generate it otherwise...

    if (true == bSuccessLoading)
        bSuccessLoading = theLedger.VerifyAccount(pServerNym);
    else
        otErr << szFunc << ": Unable to load Nymbox.\n";
    //    else
    //        bSuccessLoading        = theLedger.GenerateLedger(NYM_ID,
    // GetNotaryID(), OTLedger::nymbox, true); // bGenerateFile=true

    if (!bSuccessLoading) {
        otErr << szFunc << ": ERROR loading or generating a nymbox. (FAILED "
                           "WRITING RECEIPT!!) \n";
        return false;
    }

    OTTransaction* pTransaction = OTTransaction::GenerateTransaction(
        theLedger, OTTransaction::finalReceipt, theOriginType, lNewTransactionNumber);

    if (nullptr !=
        pTransaction) // The above has an OT_ASSERT within, but I just
                      // like to check my pointers.
    {
        pTransaction->SetOriginType(theOriginType); // Todo, verify we can just remove this line. (It's already passed in GenerateTransaction.)

        // The nymbox will get a receipt with the new transaction ID.
        // That receipt has an "in reference to" field containing the original
        // cron item.

        // set up the transaction items (each transaction may have multiple
        // items... but not in this case.)
        Item* pItem1 =
            Item::CreateItemFromTransaction(*pTransaction, Item::finalReceipt);

        // This may be unnecessary, I'll have to check
        // CreateItemFromTransaction. I'll leave it for now.
        OT_ASSERT(nullptr != pItem1);

        pItem1->SetStatus(Item::acknowledgement);

        const int64_t lOpeningNumber = GetOpeningNumber(NYM_ID);

        // Here I make sure that the receipt (the nymbox notice) references the
        // transaction number that the trader originally used to issue the cron
        // item...
        // This number is used to match up offers to trades, and used to track
        // all cron items.
        // (All Cron items require a transaction from the user to add them to
        // Cron in the
        // first place.)

        pTransaction->SetReferenceToNum(
            lOpeningNumber); // Notice this same number is set twice (again just
                             // below), so might be an opportunity to store
                             // something else in one of them.

        // The reference on the transaction contains an OTCronItem, in this
        // case.
        // The original cron item, versus the updated cron item (which is stored
        // on the finalReceipt item just below here.)
        //
        pTransaction->SetReferenceString(strOrigCronItem);

        // Normally in the Inbox, the "Closing Num" is set to the closing
        // number, in reference to the opening number. (on a finalReceipt)
        // But in the NYMBOX, we are sending the Opening Number in that spot.
        // The purpose is so the client side will know not to use that
        // opening number as a valid transaction # in its transaction statements
        // and balance statements, since the number is now gone.
        // Otherwise the Nym wouldn't know any better, and he'd keep signing for
        // it, and therefore his balance agreements would start to fail.

        pTransaction->SetClosingNum(lOpeningNumber); // This transaction is the
                                                     // finalReceipt for
                                                     // GetTransactionNum().
                                                     // (Which is also the
                                                     // original transaction
                                                     // number.)

        // The finalReceipt ITEM's NOTE contains the UPDATED CRON ITEM.
        //
        if (nullptr != pstrNote) {
            pItem1->SetNote(*pstrNote); // in markets, this is updated trade.
        }

        // Also set the ** UPDATED OFFER ** as the ATTACHMENT on the ** item.**
        // (With the SERVER's signature on it!) // in markets, this is updated
        // offer.
        //
        if (nullptr != pstrAttachment) {
            pItem1->SetAttachment(*pstrAttachment);
        }

        // sign the item

        pItem1->SignContract(pServerNym);
        pItem1->SaveContract();

        // the Transaction "owns" the item now and will handle cleaning it up.
        pTransaction->AddItem(*pItem1);

        pTransaction->SignContract(pServerNym);
        pTransaction->SaveContract();

        // Here the transaction we just created is actually added to the ledger.
        theLedger.AddTransaction(*pTransaction);

        // Release any signatures that were there before (They won't
        // verify anymore anyway, since the content has changed.)
        theLedger.ReleaseSignatures();

        // Sign and save.
        theLedger.SignContract(pServerNym);
        theLedger.SaveContract();

        // TODO: Better rollback capabilities in case of failures here:

        Identifier theNymboxHash;

        // Save nymbox to storage. (File, DB, wherever it goes.)
        theLedger.SaveNymbox(&theNymboxHash);

        // This corresponds to the AddTransaction() call just above.
        // These are stored in a separate file now.
        //
        pTransaction->SaveBoxReceipt(theLedger);

        // Update the NymboxHash (in the nymfile.)
        //

        const Identifier ACTUAL_NYM_ID = NYM_ID;
        Nym theActualNym; // unused unless it's really not already
                          // loaded. (use pActualNym.)

        // We couldn't find the Nym among those already loaded--so we have to
        // load it ourselves (so we can update its NymboxHash value.)

        if (nullptr == pActualNym) {
            if (pServerNym.CompareID(ACTUAL_NYM_ID))
                pActualNym = &pServerNym;

            else {
                theActualNym.SetIdentifier(ACTUAL_NYM_ID);

                if (!theActualNym.LoadPublicKey()) // Note: this step
                                                   // may be unnecessary
                                                   // since we are only
                                                   // updating his
                                                   // Nymfile, not his
                                                   // key.
                {
                    String strNymID(ACTUAL_NYM_ID);
                    otErr << szFunc << ": Failure loading public key for Nym: "
                          << strNymID << ". "
                                         "(To update his NymboxHash.) \n";
                }
                else if (theActualNym.VerifyPseudonym() && // this line may be unnecessary.
                           theActualNym.LoadSignedNymfile(
                               pServerNym)) // ServerNym here is not
                                             // theActualNym's identity, but
                                             // merely the signer on this file.
                {
                    otLog3 << szFunc << ": Loading actual Nym, since he wasn't "
                                        "already loaded. "
                                        "(To update his NymboxHash.)\n";
                    pActualNym = &theActualNym; //  <=====
                }
                else {
                    String strNymID(ACTUAL_NYM_ID);
                    otErr << szFunc << ": Failure loading or verifying Actual "
                                       "Nym public key: " << strNymID
                          << ". "
                             "(To update his NymboxHash.)\n";
                }
            }
        }

        // By this point we've made every possible effort to get the proper Nym
        // loaded, so that we can update his NymboxHash appropriately.
        if (nullptr != pActualNym) {
            auto context = OT::App().Contract().mutable_ClientContext(
                pServerNym.ID(), pActualNym->ID());
            context.It().SetLocalNymboxHash(theNymboxHash);
        }

        // Really this true should be predicated on ALL the above functions
        // returning true. Right?

        return true;
    } else {
        otErr << szFunc << ": Failed trying to create finalReceipt.\n";
    }

    return false; // unreachable.
}

int64_t OTCronItem::GetOpeningNum() const
{
    return GetTransactionNum();
}

int64_t OTCronItem::GetClosingNum() const
{
    return (GetCountClosingNumbers() > 0) ? GetClosingTransactionNoAt(0)
                                          : 0; // todo stop hardcoding.
}

bool OTCronItem::IsValidOpeningNumber(const int64_t& lOpeningNum) const
{
    if (GetOpeningNum() == lOpeningNum) return true;

    return false;
}

int64_t OTCronItem::GetOpeningNumber(const Identifier& theNymID) const
{
    const Identifier& theSenderNymID = GetSenderNymID();

    if (theNymID == theSenderNymID) return GetOpeningNum();

    return 0;
}

int64_t OTCronItem::GetClosingNumber(const Identifier& theAcctID) const
{
    const Identifier& theSenderAcctID = GetSenderAcctID();

    if (theAcctID == theSenderAcctID) return GetClosingNum();

    return 0;
}

// You usually wouldn't want to use this, since if the transaction failed, the
// opening number is already burned and gone. But there might be cases where
// it's not, and you want to retrieve it. So I added this function for those
// cases. In most cases, you will prefer HarvestClosingNumbers().
void OTCronItem::HarvestOpeningNumber(ServerContext& context)
{
    // The Nym is the original sender. (If Compares true). IN CASES where
    // GetTransactionNum() isn't already burned, we can harvest it here.
    // Subclasses will have to override this function for recipients, etc.
    if (context.Nym()->CompareID(GetSenderNymID())) {
        // This function will only "add it back" if it was really there in the
        // first place. (Verifies it is on issued list first, before adding to
        // available list.)
        context.RecoverAvailableNumber(GetOpeningNum());
    }

    // NOTE: if the message failed (transaction never actually ran) then the
    // sender AND recipient can both reclaim their opening numbers. But if the
    // message SUCCEEDED and the transaction FAILED, then only the recipient can
    // claim his opening number -- the sender's is already burned. So then, what
    // if you mistakenly call this function and pass the sender, when that
    // number is already burned? There's nothing this function can do, because
    // we have no way of telling, from inside here, whether the message
    // succeeded or not, and whether the transaction succeeded or not. Therefore
    // we MUST rely on the CALLER to know this, and to avoid calling this
    // function in the first place, if he's sitting on a sender with a failed
    // transaction.
}

// This is a good default implementation.
// Also, some subclasses override this, but they STILL CALL IT.
void OTCronItem::HarvestClosingNumbers(ServerContext& context)
{
    // The Nym is the original sender. (If Compares true). GetTransactionNum()
    // is usually already burned, but we can harvest the closing numbers from
    // the "Closing" list, which is only for the sender's numbers.
    // Subclasses will have to override this function for recipients, etc.
    if (context.Nym()->CompareID(GetSenderNymID())) {
        for (int32_t i = 0; i < GetCountClosingNumbers(); i++) {
            // This function will only "add it back" if it was really there in
            // the first place. (Verifies it is on issued list first, before
            // adding to available list.)
            context.RecoverAvailableNumber(GetClosingTransactionNoAt(i));
        }
    }
}

OTCronItem::OTCronItem()
    : ot_super()
    , m_pCron(nullptr)
    , serverNym_(nullptr)
    , notaryID_(nullptr)
    , m_CREATION_DATE(OT_TIME_ZERO)
    , m_LAST_PROCESS_DATE(OT_TIME_ZERO)
    , m_PROCESS_INTERVAL(1)
    , // Default for any cron item is to execute once per second.
    m_pCancelerNymID(new Identifier)
    , m_bCanceled(false)
    , m_bRemovalFlag(false)
{
    InitCronItem();
}

OTCronItem::OTCronItem(const Identifier& NOTARY_ID,
                       const Identifier& INSTRUMENT_DEFINITION_ID)
    : ot_super(NOTARY_ID, INSTRUMENT_DEFINITION_ID)
    , m_pCron(nullptr)
    , serverNym_(nullptr)
    , notaryID_(nullptr)
    , m_CREATION_DATE(OT_TIME_ZERO)
    , m_LAST_PROCESS_DATE(OT_TIME_ZERO)
    , m_PROCESS_INTERVAL(1)
    , // Default for any cron item is to execute once per second.
    m_pCancelerNymID(new Identifier)
    , m_bCanceled(false)
    , m_bRemovalFlag(false)
{
    InitCronItem();
}

OTCronItem::OTCronItem(const Identifier& NOTARY_ID,
                       const Identifier& INSTRUMENT_DEFINITION_ID,
                       const Identifier& ACCT_ID, const Identifier& NYM_ID)
    : ot_super(NOTARY_ID, INSTRUMENT_DEFINITION_ID, ACCT_ID, NYM_ID)
    , m_pCron(nullptr)
    , serverNym_(nullptr)
    , notaryID_(nullptr)
    , m_CREATION_DATE(OT_TIME_ZERO)
    , m_LAST_PROCESS_DATE(OT_TIME_ZERO)
    , m_PROCESS_INTERVAL(1)
    , // Default for any cron item is to execute once per second.
    m_pCancelerNymID(new Identifier)
    , m_bCanceled(false)
    , m_bRemovalFlag(false)

{
    InitCronItem();
}

bool OTCronItem::GetCancelerID(Identifier& theOutput) const
{
    if (!IsCanceled()) {
        theOutput.Release();
        return false;
    }

    theOutput = *m_pCancelerNymID;
    return true;
}

// When canceling a cron item before it has been activated, use this.
//
bool OTCronItem::CancelBeforeActivation(Nym& theCancelerNym)
{
    OT_ASSERT(nullptr != m_pCancelerNymID);

    if (IsCanceled()) return false;

    m_bCanceled = true;
    *m_pCancelerNymID = theCancelerNym.GetConstID();

    ReleaseSignatures();
    SignContract(theCancelerNym);
    SaveContract();

    return true;
}

void OTCronItem::InitCronItem()
{
    m_strContractType.Set("CRONITEM"); // in practice should never appear. Child
                                       // classes will overwrite.
}

void OTCronItem::ClearClosingNumbers()
{
    m_dequeClosingNumbers.clear();
}

OTCronItem::~OTCronItem()
{
    Release_CronItem();

    // If there were any dynamically allocated objects, clean them up here.
    //
    if (nullptr != m_pCancelerNymID) delete m_pCancelerNymID;
    m_pCancelerNymID = nullptr;
}

void OTCronItem::Release_CronItem()
{
    m_CREATION_DATE = OT_TIME_ZERO;
    m_LAST_PROCESS_DATE = OT_TIME_ZERO;
    m_PROCESS_INTERVAL = 1;

    ClearClosingNumbers();

    m_bRemovalFlag = false;
    m_bCanceled = false;
    m_pCancelerNymID->Release();
}

void OTCronItem::Release()
{
    Release_CronItem();

    ot_super::Release(); // since I've overridden the base class, I call it
                         // now...
}

// return -1 if error, 0 if nothing, and 1 if the node was processed.
int32_t OTCronItem::ProcessXMLNode(irr::io::IrrXMLReader*& xml)
{
    int32_t nReturnVal = 0;

    // Here we call the parent class first.
    // If the node is found there, or there is some error,
    // then we just return either way.  But if it comes back
    // as '0', then nothing happened, and we'll continue executing.
    //
    // -- Note you can choose not to call the parent if
    // you don't want to use any of those xml tags.
    //

    nReturnVal = ot_super::ProcessXMLNode(xml);

    if (nReturnVal !=
        0) // -1 is error, and 1 is "found it". Either way, return.
        return nReturnVal; // 0 means "nothing happened, keep going."

    const String strNodeName(xml->getNodeName());

    if (strNodeName.Compare("closingTransactionNumber")) {
        String strClosingNumber = xml->getAttributeValue("value");

        if (strClosingNumber.Exists()) {
            const int64_t lClosingNumber = strClosingNumber.ToLong();

            AddClosingTransactionNo(lClosingNumber);
        }
        else {
            otErr << "Error in OTCronItem::ProcessXMLNode: "
                     "closingTransactionNumber field without value.\n";
            return (-1); // error condition
        }

        nReturnVal = 1;
    }

    return nReturnVal;
}

} // namespace opentxs
