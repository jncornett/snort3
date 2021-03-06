//--------------------------------------------------------------------------
// Copyright (C) 2014-2015 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
// nhttp_msg_section.cc author Tom Peters <thopeter@cisco.com>

#include <string.h>
#include <sys/types.h>
#include <stdio.h>

#include "detection/detection_util.h"

#include "nhttp_enum.h"
#include "nhttp_transaction.h"
#include "nhttp_msg_section.h"
#include "nhttp_msg_request.h"
#include "nhttp_msg_status.h"
#include "nhttp_msg_head_shared.h"
#include "nhttp_msg_body.h"

using namespace NHttpEnums;

NHttpMsgSection::NHttpMsgSection(const uint8_t* buffer, const uint16_t buf_size,
       NHttpFlowData* session_data_, SourceId source_id_, bool buf_owner) :
    msg_text(buf_size, buffer),
    session_data(session_data_),
    source_id(source_id_),
    transaction(NHttpTransaction::attach_my_transaction(session_data, source_id)),
    tcp_close(session_data->tcp_close[source_id]),
    scratch_pad(2*buf_size+500),
    infractions(session_data->infractions[source_id]),
    events(session_data->events[source_id]),
    version_id(session_data->version_id[source_id]),
    method_id((source_id == SRC_CLIENT) ? session_data->method_id : METH__NOTPRESENT),
    status_code_num((source_id == SRC_SERVER) ? session_data->status_code_num : STAT_NOTPRESENT),
    delete_msg_on_destruct(buf_owner)
{ }

void NHttpMsgSection::print_message_title(FILE* output, const char* title) const
{
    fprintf(output, "HTTP message %s:\n", title);
    msg_text.print(output, "Input");
}

void NHttpMsgSection::print_message_wrapup(FILE* output)
{
    fprintf(output, "Infractions: %" PRIx64 ", Events: %" PRIx64 ", TCP Close: %s\n",
        infractions.get_raw(), events.get_raw(), tcp_close ? "True" : "False");
    for (unsigned k=1; k < HTTP_BUFFER_MAX; k++)
    {
        get_legacy(k).print(output, http_buffer_name[k]);
    }
    if (g_file_data.len > 0)
    {
        Field(g_file_data.len, g_file_data.data).print(output, "file_data");
    }
    fprintf(output, "\n");
    session_data->show(output);
    fprintf(output, "\n");
}

const Field& NHttpMsgSection::get_legacy(unsigned buffer_id)
{
    // When current section is trailers, that is what will be used for header and cookie buffers.
    switch (buffer_id)
    {
    case HTTP_BUFFER_CLIENT_BODY:
      {
        NHttpMsgBody* body = transaction->get_body();
        return (body != nullptr) ? body->get_data() : Field::FIELD_NULL;
      }
    case HTTP_BUFFER_COOKIE:
      {
        NHttpMsgHeadShared* header = transaction->get_latest_header(source_id);
        if (header == nullptr)
            return Field::FIELD_NULL;
        HeaderId cookie_head = (source_id == SRC_CLIENT) ? HEAD_COOKIE : HEAD_SET_COOKIE;
        return header->get_header_value_norm(cookie_head);
      }
    case HTTP_BUFFER_HEADER:
      {
        NHttpMsgHeadShared* header = transaction->get_latest_header(source_id);
        return (header != nullptr) ? header->get_headers() : Field::FIELD_NULL;
      }
    case HTTP_BUFFER_METHOD:
      {
        NHttpMsgRequest* request = transaction->get_request();
        return (request != nullptr) ? request->get_method() : Field::FIELD_NULL;
      }
    case HTTP_BUFFER_RAW_COOKIE:
      {
        NHttpMsgHeadShared* header = transaction->get_latest_header(source_id);
        if (header == nullptr)
            return Field::FIELD_NULL;
        HeaderId cookie_head = (source_id == SRC_CLIENT) ? HEAD_COOKIE : HEAD_SET_COOKIE;
        // FIXIT-M there can be multiple cookie header in one message.
        for (int k=0; k < header->get_num_headers(); k++)
        {
            if (header->get_header_name_id(k) == cookie_head)
            {
                return header->get_header_value(k);
            }
        }
        return Field::FIELD_NULL;
      }
    case HTTP_BUFFER_RAW_HEADER:
      {
        NHttpMsgHeadShared* header = transaction->get_latest_header(source_id);
        return (header != nullptr) ? header->get_headers() : Field::FIELD_NULL;
      }
    case HTTP_BUFFER_RAW_URI:
      {
        NHttpMsgRequest* request = transaction->get_request();
        return (request != nullptr) ? request->get_uri() : Field::FIELD_NULL;
      }
    case HTTP_BUFFER_STAT_CODE:
      {
        NHttpMsgStatus* status = transaction->get_status();
        return (status != nullptr) ? status->get_status_code() : Field::FIELD_NULL;
      }
    case HTTP_BUFFER_STAT_MSG:
      {
        NHttpMsgStatus* status = transaction->get_status();
        return (status != nullptr) ? status->get_reason_phrase() : Field::FIELD_NULL;
      }
    case HTTP_BUFFER_URI:
      {
        NHttpMsgRequest* request = transaction->get_request();
        return (request != nullptr) ? request->get_uri_norm_legacy() : Field::FIELD_NULL;
      }
    default:
        assert(0);
        return Field::FIELD_NULL;
    }
}

