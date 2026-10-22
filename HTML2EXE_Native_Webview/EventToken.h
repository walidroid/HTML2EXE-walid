// EventToken.h
// MinGW-w64 compatibility header for Microsoft WebView2.
// Defines EventRegistrationToken required by WebView2.h.

#ifndef __eventtoken_h__
#define __eventtoken_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef __EventRegistrationToken_FWD_DEFINED__
#define __EventRegistrationToken_FWD_DEFINED__
typedef struct EventRegistrationToken EventRegistrationToken;
#endif /* __EventRegistrationToken_FWD_DEFINED__ */

#ifndef __EventRegistrationToken_DEFINED__
#define __EventRegistrationToken_DEFINED__

typedef struct EventRegistrationToken
{
    __int64 value;
} EventRegistrationToken;

#endif /* __EventRegistrationToken_DEFINED__ */

#endif /* __eventtoken_h__ */
