# Copyright (c) 2013, AllSeen Alliance. All rights reserved.
#
#    Permission to use, copy, modify, and/or distribute this software for any
#    purpose with or without fee is hereby granted, provided that the above
#    copyright notice and this permission notice appear in all copies.
#
#    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import os

Import('env')

env['_ALLJOYN_SERVICES_COMMON_'] = True

# Make config library and header file paths available to the global environment
env.Append(LIBPATH = '$DISTDIR/services_common/lib');
env.Append(CPPPATH = '$DISTDIR/services_common/inc');

if 'cpp' in env['bindings'] and not env.has_key('_ALLJOYNCORE_') and os.path.exists('../../alljoyn_core/SConscript'):
    env.SConscript('../../alljoyn_core/SConscript')

if 'java' in env['bindings'] and not env.has_key('_ALLJOYN_JAVA_') and os.path.exists('../../alljoyn_java/SConscript'):
    env.SConscript('../../alljoyn_java/SConscript')

services_common_env = env.Clone()

for b in services_common_env['bindings']:
    if os.path.exists('%s/SConscript' % b):
        services_common_env.VariantDir('$OBJDIR/%s' % b, b, duplicate = 0)

services_common_env.SConscript(['$OBJDIR/%s/SConscript' % b for b in env['bindings'] if os.path.exists('%s/SConscript' % b) ],
                      exports = ['services_common_env'])

