/*
  Copyright (c) 2015 StarBrilliant <m13253@hotmail.com>
  All rights reserved.

  Redistribution and use in source and binary forms are permitted
  provided that the above copyright notice and this paragraph are
  duplicated in all such forms and that any documentation,
  advertising materials, and other materials related to such
  distribution and use acknowledge that the software was developed by
  StarBrilliant.
  The name of StarBrilliant may not be used to endorse or promote
  products derived from this software without specific prior written
  permission.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#pragma once

#include "../utils.h"
#include "../app.h"
#include "../renderer/danmaku_entry.h"
#include <functional>

namespace dmhm {

class ConsoleFetcher {

public:

    ConsoleFetcher(Application *app);
    ~ConsoleFetcher();
    void run_thread();
    bool is_eof();
    void pop_messages(std::function<void (DanmakuEntry &entry)> callback);

private:

    proxy_ptr<struct ConsoleFetcherPrivate> p;

};

}
