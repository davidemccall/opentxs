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

#include "opentxs/core/Instrument.hpp"

#include "opentxs/core/script/OTScriptable.hpp"
#include "opentxs/core/util/Common.hpp"
#include "opentxs/core/Contract.hpp"
#include "opentxs/core/Identifier.hpp"
#include "opentxs/core/String.hpp"

#include <stdint.h>

namespace opentxs
{

// Verify whether the CURRENT date is AFTER the the VALID TO date.
// Notice, this will return false, if the instrument is NOT YET VALID.
// You have to use VerifyCurrentDate() to make sure you're within the
// valid date range to use this instrument. But sometimes you only want
// to know if it's expired, regardless of whether it's valid yet. So this
// function answers that for you.
bool Instrument::IsExpired()
{
    const time64_t CURRENT_TIME = OTTimeGetCurrentTime();

    // If the current time is AFTER the valid-TO date,
    // AND the valid_to is a nonzero number (0 means "doesn't expire")
    // THEN return true (it's expired.)
    //
    if ((CURRENT_TIME >= m_VALID_TO) && (m_VALID_TO > OT_TIME_ZERO))
        return true;
    else
        return false;
}

// Verify whether the CURRENT date is WITHIN the VALID FROM / TO dates.
bool Instrument::VerifyCurrentDate()
{
    const time64_t CURRENT_TIME = OTTimeGetCurrentTime();

    if ((CURRENT_TIME >= m_VALID_FROM) &&
        ((CURRENT_TIME <= m_VALID_TO) || (OT_TIME_ZERO == m_VALID_TO)))
        return true;
    else
        return false;
}

void Instrument::InitInstrument()
{
    m_strContractType.Set("INSTRUMENT");
}

Instrument::Instrument()
    : OTScriptable()
    , m_VALID_FROM(OT_TIME_ZERO)
    , m_VALID_TO(OT_TIME_ZERO)
{
    InitInstrument();
}

Instrument::Instrument(const Identifier& NOTARY_ID,
                       const Identifier& INSTRUMENT_DEFINITION_ID)
    : OTScriptable()
    , m_InstrumentDefinitionID(INSTRUMENT_DEFINITION_ID)
    , m_NotaryID(NOTARY_ID)
    , m_VALID_FROM(OT_TIME_ZERO)
    , m_VALID_TO(OT_TIME_ZERO)
{
    InitInstrument();
}

Instrument::~Instrument()
{
    Release_Instrument();

    m_VALID_FROM = OT_TIME_ZERO;
    m_VALID_TO = OT_TIME_ZERO;
}

void Instrument::Release_Instrument()
{
    // Release any dynamically allocated instrument members here.
}

void Instrument::Release()
{
    Release_Instrument(); // My own cleanup is performed here.
    // Next give the base class a chance to do the same...
    // since I've overridden the base class, I call it now
    OTScriptable::Release();
    // Initialize everything back to 0
    //    InitInstrument(); // unnecessary.
}

// return -1 if error, 0 if nothing, and 1 if the node was processed.
int32_t Instrument::ProcessXMLNode(irr::io::IrrXMLReader*& xml)
{
    //    otErr << "OTInstrument::ProcessXMLNode...\n";
    int32_t nReturnVal = 0;

    // Here we call the parent class first.
    // If the node is found there, or there is some error,
    // then we just return either way.  But if it comes back
    // as '0', then nothing happened, and we'll continue executing.
    //
    // -- Note you can choose not to call the parent if
    // you don't want to use any of those xml tags.
    //

    nReturnVal = OTScriptable::ProcessXMLNode(xml);

    // -1 is error, and 1 is "found it". Either way, return.
    if (nReturnVal != 0) {
        return nReturnVal; // 0 means "nothing happened, keep going."
    }
    // This is from OTCronItem. It's only here as sample code.
    //
    //  if (!strcmp("closingTransactionNumber", xml->getNodeName()))
    //    {
    //        OTString strClosingNumber = xml->getAttributeValue("value");
    //
    //        if (strClosingNumber.Exists())
    //        {
    //            const int64_t lClosingNumber = atol(strClosingNumber.Get());
    //
    //            AddClosingTransactionNo(lClosingNumber);
    //        }
    //        else
    //        {
    //            otErr << "Error in OTCronItem::ProcessXMLNode:
    // closingTransactionNumber field without value.\n";
    //            return (-1); // error condition
    //        }
    //
    //        nReturnVal = 1;
    //    }

    return nReturnVal;
}

} // namespace opentxs
