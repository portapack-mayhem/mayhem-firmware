/*
 * Copyright (C) 2014 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __THREAD_BASE_H__
#define __THREAD_BASE_H__

#include <ch.h>

class ThreadBase {
   public:
    virtual ~ThreadBase() = default;

    virtual void start() = 0;

   protected:
    static msg_t fn(void* arg) {
        auto obj = static_cast<ThreadBase*>(arg);
        obj->run();

        return 0;
    }

   private:
    virtual void run() = 0;
};

#endif /*__THREAD_BASE_H__*/
