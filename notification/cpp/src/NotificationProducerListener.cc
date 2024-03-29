/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#include "NotificationProducerListener.h"
#include <iostream>
#include <algorithm>
#include <alljoyn/notification/NotificationService.h>
#include "NotificationConstants.h"
#include <alljoyn/notification/LogModule.h>

using namespace ajn;
using namespace services;
using namespace qcc;
using namespace nsConsts;

NotificationProducerListener::NotificationProducerListener() :
    SessionPortListener(), m_SessionPort(0)
{
}

NotificationProducerListener::~NotificationProducerListener()
{
}

void NotificationProducerListener::setSessionPort(ajn::SessionPort sessionPort)
{
    m_SessionPort = sessionPort;
}

SessionPort NotificationProducerListener::getSessionPort()
{
    return m_SessionPort;
}

bool NotificationProducerListener::AcceptSessionJoiner(ajn::SessionPort sessionPort, const char* joiner, const ajn::SessionOpts& opts)
{
    QCC_UNUSED(joiner); //joiner only used in debug build not release
    QCC_UNUSED(opts);
    if (sessionPort != m_SessionPort) {
        return false;
    }

    QCC_DbgPrintf(("NotificationProducerListener::AcceptSessionJoiner() sessionPort=%hu joiner:%s", sessionPort, joiner));
    return true;
}

void NotificationProducerListener::SessionJoined(SessionPort sessionPort, SessionId sessionId, const char* joiner)
{
    QCC_UNUSED(sessionPort);
    QCC_UNUSED(sessionId);
    QCC_UNUSED(joiner);
}
