/*
 * Copyright (c) 2026, Gianluigi Tiesi <sherpya@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 */

/* nl_exports table for the bcd fake-DLL. Keep this TU header-light: the
 * NL_EXPORT macro re-declares each symbol as `extern char[]`, which would
 * collide with a real prototype if one were in scope. */
#include "../../nt_structs.h"

NL_EXPORT(bcd, BcdOpenStore)
NL_EXPORT(bcd, BcdCloseStore)
NL_EXPORT(bcd, BcdForciblyUnloadStore)
NL_EXPORT(bcd, BcdOpenObject)
NL_EXPORT(bcd, BcdCloseObject)
NL_EXPORT(bcd, BcdGetElementData)
