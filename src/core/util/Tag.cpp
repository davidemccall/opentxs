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

#include "opentxs/core/util/Tag.hpp"

#include <memory>
#include <string>
#include <utility>

namespace opentxs
{

void Tag::add_attribute(const std::string& str_att_name,
                        const char* sz_att_value)
{
    std::string str_temp(sz_att_value);

    add_attribute(str_att_name, str_temp);
}

void Tag::add_attribute(const std::string& str_att_name,
                        const std::string& str_att_value)
{
    std::pair<std::string, std::string> temp =
        std::make_pair(str_att_name, str_att_value);

    attributes_.insert(temp);
}

void Tag::output(std::string& str_output) const
{
    outputXML(str_output);
}

void Tag::outputXML(std::string& str_output) const
{
    str_output += ("<" + name_);

    if (!attributes_.empty()) {
        for (auto& kv : attributes_) {
            str_output += ("\n " + kv.first + "=\"" + kv.second + "\"");
        }
    }

    if (text_.empty() && tags_.empty()) {
        str_output += " />\n";
    }
    else {
        str_output += ">\n";

        if (!text_.empty()) {
            str_output += text_;
        }
        else if (!tags_.empty()) {
            for (auto& kv : tags_) {
                kv->output(str_output);
            }
        }

        str_output += ("\n</" + name_ + ">\n");
    }
}

void Tag::add_tag(TagPtr& tag_input)
{
    tags_.push_back(tag_input);
}

void Tag::add_tag(const std::string& str_tag_name,
                  const std::string& str_tag_value)
{
    TagPtr p1 = std::make_shared<Tag>(str_tag_name, str_tag_value);

    add_tag(p1);
}

Tag::Tag(const std::string& str_name)
    : name_(str_name)
{
}

Tag::Tag(const std::string& str_name, const std::string& str_text)
    : name_(str_name)
    , text_(str_text)
{
}

Tag::Tag(const std::string& str_name, const char* sztext)
    : name_(str_name)
    , text_(sztext)
{
}

} // namespace
