// -------------------------------------------------------------------------------------------------
//  File idn.h
//
//  Copyright (c) 2013-2019 DexLogic, Dirk Apitz
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//
// -------------------------------------------------------------------------------------------------
//  Change History:
//
//  07/2013 Dirk Apitz, created
//  01/2015 Dirk Apitz, IDN standard draft, separate stream header file
//  05/2016 Dirk Apitz, separate hello header file
//  06/2017 Dirk Apitz, moved udp port to hello header
//
//  Version: 2019-07-24
// -------------------------------------------------------------------------------------------------


#ifndef IDN_H
#define IDN_H


// Standard libraries
#include <stdint.h>


// -------------------------------------------------------------------------------------------------
//  Messages
// -------------------------------------------------------------------------------------------------

typedef struct _IDNHDR_MESSAGE
{
    uint16_t totalSize;
    uint16_t contentID;

} IDNHDR_MESSAGE;



#endif
