/*
* Copyright 2015-2021 Esri
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: Lucian Plesea
*
*/

#include "marfa.h"

#if defined(JPEG12_SUPPORTED)

#if !defined(HAVE_JPEGTURBO_DUAL_MODE_8_12)
// Including LIBJPEG_12_H preculdes libjpeg.h from being read again
CPL_C_START
#include "../jpeg/libjpeg12/jpeglib.h"
CPL_C_END
#endif

#define  JPEG12_ON

#include "JPEG_band.cpp"

#endif // JPEG12_SUPPORTED
