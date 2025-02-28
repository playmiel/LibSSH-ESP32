/*
 * dh.c - Diffie-Helman algorithm code against SSH 2
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2018 by Aris Adamantiadis
 * Copyright (c) 2009-2013 by Andreas Schneider <asn@cryptomilk.org>
 * Copyright (c) 2012      by Dmitriy Kuznetsov <dk@yandex.ru>
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "libssh_esp32_config.h"

#include "libssh/priv.h"
#include "libssh/crypto.h"
#include "libssh/buffer.h"
#include "libssh/session.h"
#include "libssh/misc.h"
#include "libssh/dh.h"
#include "libssh/ssh2.h"
#include "libssh/pki.h"
#include "libssh/bignum.h"

static unsigned char p_group1_value[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2,
        0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
        0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6,
        0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
        0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
        0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
        0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9,
        0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
        0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11,
        0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE6, 0x53, 0x81,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define P_GROUP1_LEN 128	/* Size in bytes of the p number */

static unsigned char p_group14_value[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2,
        0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
        0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6,
        0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
        0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
        0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
        0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9,
        0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
        0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11,
        0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
        0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05, 0x98, 0xDA, 0x48, 0x36,
        0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
        0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56,
        0x20, 0x85, 0x52, 0xBB, 0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,
        0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04, 0xF1, 0x74, 0x6C, 0x08,
        0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
        0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2,
        0xEC, 0x07, 0xA2, 0x8F, 0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9,
        0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18, 0x39, 0x95, 0x49, 0x7C,
        0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
        0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAC, 0xAA, 0x68, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF};

#define P_GROUP14_LEN 256 /* Size in bytes of the p number for group 14 */

static unsigned char p_group16_value[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2,
    0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
    0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6,
    0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
    0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
    0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9,
    0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11,
    0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
    0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05, 0x98, 0xDA, 0x48, 0x36,
    0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
    0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56,
    0x20, 0x85, 0x52, 0xBB, 0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,
    0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04, 0xF1, 0x74, 0x6C, 0x08,
    0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
    0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2,
    0xEC, 0x07, 0xA2, 0x8F, 0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9,
    0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18, 0x39, 0x95, 0x49, 0x7C,
    0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
    0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAA, 0xC4, 0x2D, 0xAD, 0x33, 0x17, 0x0D,
    0x04, 0x50, 0x7A, 0x33, 0xA8, 0x55, 0x21, 0xAB, 0xDF, 0x1C, 0xBA, 0x64,
    0xEC, 0xFB, 0x85, 0x04, 0x58, 0xDB, 0xEF, 0x0A, 0x8A, 0xEA, 0x71, 0x57,
    0x5D, 0x06, 0x0C, 0x7D, 0xB3, 0x97, 0x0F, 0x85, 0xA6, 0xE1, 0xE4, 0xC7,
    0xAB, 0xF5, 0xAE, 0x8C, 0xDB, 0x09, 0x33, 0xD7, 0x1E, 0x8C, 0x94, 0xE0,
    0x4A, 0x25, 0x61, 0x9D, 0xCE, 0xE3, 0xD2, 0x26, 0x1A, 0xD2, 0xEE, 0x6B,
    0xF1, 0x2F, 0xFA, 0x06, 0xD9, 0x8A, 0x08, 0x64, 0xD8, 0x76, 0x02, 0x73,
    0x3E, 0xC8, 0x6A, 0x64, 0x52, 0x1F, 0x2B, 0x18, 0x17, 0x7B, 0x20, 0x0C,
    0xBB, 0xE1, 0x17, 0x57, 0x7A, 0x61, 0x5D, 0x6C, 0x77, 0x09, 0x88, 0xC0,
    0xBA, 0xD9, 0x46, 0xE2, 0x08, 0xE2, 0x4F, 0xA0, 0x74, 0xE5, 0xAB, 0x31,
    0x43, 0xDB, 0x5B, 0xFC, 0xE0, 0xFD, 0x10, 0x8E, 0x4B, 0x82, 0xD1, 0x20,
    0xA9, 0x21, 0x08, 0x01, 0x1A, 0x72, 0x3C, 0x12, 0xA7, 0x87, 0xE6, 0xD7,
    0x88, 0x71, 0x9A, 0x10, 0xBD, 0xBA, 0x5B, 0x26, 0x99, 0xC3, 0x27, 0x18,
    0x6A, 0xF4, 0xE2, 0x3C, 0x1A, 0x94, 0x68, 0x34, 0xB6, 0x15, 0x0B, 0xDA,
    0x25, 0x83, 0xE9, 0xCA, 0x2A, 0xD4, 0x4C, 0xE8, 0xDB, 0xBB, 0xC2, 0xDB,
    0x04, 0xDE, 0x8E, 0xF9, 0x2E, 0x8E, 0xFC, 0x14, 0x1F, 0xBE, 0xCA, 0xA6,
    0x28, 0x7C, 0x59, 0x47, 0x4E, 0x6B, 0xC0, 0x5D, 0x99, 0xB2, 0x96, 0x4F,
    0xA0, 0x90, 0xC3, 0xA2, 0x23, 0x3B, 0xA1, 0x86, 0x51, 0x5B, 0xE7, 0xED,
    0x1F, 0x61, 0x29, 0x70, 0xCE, 0xE2, 0xD7, 0xAF, 0xB8, 0x1B, 0xDD, 0x76,
    0x21, 0x70, 0x48, 0x1C, 0xD0, 0x06, 0x91, 0x27, 0xD5, 0xB0, 0x5A, 0xA9,
    0x93, 0xB4, 0xEA, 0x98, 0x8D, 0x8F, 0xDD, 0xC1, 0x86, 0xFF, 0xB7, 0xDC,
    0x90, 0xA6, 0xC0, 0x8F, 0x4D, 0xF4, 0x35, 0xC9, 0x34, 0x06, 0x31, 0x99,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define P_GROUP16_LEN 512 /* Size in bytes of the p number for group 16 */

static unsigned char p_group18_value[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2,
    0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
    0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6,
    0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
    0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
    0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
    0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9,
    0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
    0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11,
    0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE4, 0x5B, 0x3D,
    0xC2, 0x00, 0x7C, 0xB8, 0xA1, 0x63, 0xBF, 0x05, 0x98, 0xDA, 0x48, 0x36,
    0x1C, 0x55, 0xD3, 0x9A, 0x69, 0x16, 0x3F, 0xA8, 0xFD, 0x24, 0xCF, 0x5F,
    0x83, 0x65, 0x5D, 0x23, 0xDC, 0xA3, 0xAD, 0x96, 0x1C, 0x62, 0xF3, 0x56,
    0x20, 0x85, 0x52, 0xBB, 0x9E, 0xD5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6D,
    0x67, 0x0C, 0x35, 0x4E, 0x4A, 0xBC, 0x98, 0x04, 0xF1, 0x74, 0x6C, 0x08,
    0xCA, 0x18, 0x21, 0x7C, 0x32, 0x90, 0x5E, 0x46, 0x2E, 0x36, 0xCE, 0x3B,
    0xE3, 0x9E, 0x77, 0x2C, 0x18, 0x0E, 0x86, 0x03, 0x9B, 0x27, 0x83, 0xA2,
    0xEC, 0x07, 0xA2, 0x8F, 0xB5, 0xC5, 0x5D, 0xF0, 0x6F, 0x4C, 0x52, 0xC9,
    0xDE, 0x2B, 0xCB, 0xF6, 0x95, 0x58, 0x17, 0x18, 0x39, 0x95, 0x49, 0x7C,
    0xEA, 0x95, 0x6A, 0xE5, 0x15, 0xD2, 0x26, 0x18, 0x98, 0xFA, 0x05, 0x10,
    0x15, 0x72, 0x8E, 0x5A, 0x8A, 0xAA, 0xC4, 0x2D, 0xAD, 0x33, 0x17, 0x0D,
    0x04, 0x50, 0x7A, 0x33, 0xA8, 0x55, 0x21, 0xAB, 0xDF, 0x1C, 0xBA, 0x64,
    0xEC, 0xFB, 0x85, 0x04, 0x58, 0xDB, 0xEF, 0x0A, 0x8A, 0xEA, 0x71, 0x57,
    0x5D, 0x06, 0x0C, 0x7D, 0xB3, 0x97, 0x0F, 0x85, 0xA6, 0xE1, 0xE4, 0xC7,
    0xAB, 0xF5, 0xAE, 0x8C, 0xDB, 0x09, 0x33, 0xD7, 0x1E, 0x8C, 0x94, 0xE0,
    0x4A, 0x25, 0x61, 0x9D, 0xCE, 0xE3, 0xD2, 0x26, 0x1A, 0xD2, 0xEE, 0x6B,
    0xF1, 0x2F, 0xFA, 0x06, 0xD9, 0x8A, 0x08, 0x64, 0xD8, 0x76, 0x02, 0x73,
    0x3E, 0xC8, 0x6A, 0x64, 0x52, 0x1F, 0x2B, 0x18, 0x17, 0x7B, 0x20, 0x0C,
    0xBB, 0xE1, 0x17, 0x57, 0x7A, 0x61, 0x5D, 0x6C, 0x77, 0x09, 0x88, 0xC0,
    0xBA, 0xD9, 0x46, 0xE2, 0x08, 0xE2, 0x4F, 0xA0, 0x74, 0xE5, 0xAB, 0x31,
    0x43, 0xDB, 0x5B, 0xFC, 0xE0, 0xFD, 0x10, 0x8E, 0x4B, 0x82, 0xD1, 0x20,
    0xA9, 0x21, 0x08, 0x01, 0x1A, 0x72, 0x3C, 0x12, 0xA7, 0x87, 0xE6, 0xD7,
    0x88, 0x71, 0x9A, 0x10, 0xBD, 0xBA, 0x5B, 0x26, 0x99, 0xC3, 0x27, 0x18,
    0x6A, 0xF4, 0xE2, 0x3C, 0x1A, 0x94, 0x68, 0x34, 0xB6, 0x15, 0x0B, 0xDA,
    0x25, 0x83, 0xE9, 0xCA, 0x2A, 0xD4, 0x4C, 0xE8, 0xDB, 0xBB, 0xC2, 0xDB,
    0x04, 0xDE, 0x8E, 0xF9, 0x2E, 0x8E, 0xFC, 0x14, 0x1F, 0xBE, 0xCA, 0xA6,
    0x28, 0x7C, 0x59, 0x47, 0x4E, 0x6B, 0xC0, 0x5D, 0x99, 0xB2, 0x96, 0x4F,
    0xA0, 0x90, 0xC3, 0xA2, 0x23, 0x3B, 0xA1, 0x86, 0x51, 0x5B, 0xE7, 0xED,
    0x1F, 0x61, 0x29, 0x70, 0xCE, 0xE2, 0xD7, 0xAF, 0xB8, 0x1B, 0xDD, 0x76,
    0x21, 0x70, 0x48, 0x1C, 0xD0, 0x06, 0x91, 0x27, 0xD5, 0xB0, 0x5A, 0xA9,
    0x93, 0xB4, 0xEA, 0x98, 0x8D, 0x8F, 0xDD, 0xC1, 0x86, 0xFF, 0xB7, 0xDC,
    0x90, 0xA6, 0xC0, 0x8F, 0x4D, 0xF4, 0x35, 0xC9, 0x34, 0x02, 0x84, 0x92,
    0x36, 0xC3, 0xFA, 0xB4, 0xD2, 0x7C, 0x70, 0x26, 0xC1, 0xD4, 0xDC, 0xB2,
    0x60, 0x26, 0x46, 0xDE, 0xC9, 0x75, 0x1E, 0x76, 0x3D, 0xBA, 0x37, 0xBD,
    0xF8, 0xFF, 0x94, 0x06, 0xAD, 0x9E, 0x53, 0x0E, 0xE5, 0xDB, 0x38, 0x2F,
    0x41, 0x30, 0x01, 0xAE, 0xB0, 0x6A, 0x53, 0xED, 0x90, 0x27, 0xD8, 0x31,
    0x17, 0x97, 0x27, 0xB0, 0x86, 0x5A, 0x89, 0x18, 0xDA, 0x3E, 0xDB, 0xEB,
    0xCF, 0x9B, 0x14, 0xED, 0x44, 0xCE, 0x6C, 0xBA, 0xCE, 0xD4, 0xBB, 0x1B,
    0xDB, 0x7F, 0x14, 0x47, 0xE6, 0xCC, 0x25, 0x4B, 0x33, 0x20, 0x51, 0x51,
    0x2B, 0xD7, 0xAF, 0x42, 0x6F, 0xB8, 0xF4, 0x01, 0x37, 0x8C, 0xD2, 0xBF,
    0x59, 0x83, 0xCA, 0x01, 0xC6, 0x4B, 0x92, 0xEC, 0xF0, 0x32, 0xEA, 0x15,
    0xD1, 0x72, 0x1D, 0x03, 0xF4, 0x82, 0xD7, 0xCE, 0x6E, 0x74, 0xFE, 0xF6,
    0xD5, 0x5E, 0x70, 0x2F, 0x46, 0x98, 0x0C, 0x82, 0xB5, 0xA8, 0x40, 0x31,
    0x90, 0x0B, 0x1C, 0x9E, 0x59, 0xE7, 0xC9, 0x7F, 0xBE, 0xC7, 0xE8, 0xF3,
    0x23, 0xA9, 0x7A, 0x7E, 0x36, 0xCC, 0x88, 0xBE, 0x0F, 0x1D, 0x45, 0xB7,
    0xFF, 0x58, 0x5A, 0xC5, 0x4B, 0xD4, 0x07, 0xB2, 0x2B, 0x41, 0x54, 0xAA,
    0xCC, 0x8F, 0x6D, 0x7E, 0xBF, 0x48, 0xE1, 0xD8, 0x14, 0xCC, 0x5E, 0xD2,
    0x0F, 0x80, 0x37, 0xE0, 0xA7, 0x97, 0x15, 0xEE, 0xF2, 0x9B, 0xE3, 0x28,
    0x06, 0xA1, 0xD5, 0x8B, 0xB7, 0xC5, 0xDA, 0x76, 0xF5, 0x50, 0xAA, 0x3D,
    0x8A, 0x1F, 0xBF, 0xF0, 0xEB, 0x19, 0xCC, 0xB1, 0xA3, 0x13, 0xD5, 0x5C,
    0xDA, 0x56, 0xC9, 0xEC, 0x2E, 0xF2, 0x96, 0x32, 0x38, 0x7F, 0xE8, 0xD7,
    0x6E, 0x3C, 0x04, 0x68, 0x04, 0x3E, 0x8F, 0x66, 0x3F, 0x48, 0x60, 0xEE,
    0x12, 0xBF, 0x2D, 0x5B, 0x0B, 0x74, 0x74, 0xD6, 0xE6, 0x94, 0xF9, 0x1E,
    0x6D, 0xBE, 0x11, 0x59, 0x74, 0xA3, 0x92, 0x6F, 0x12, 0xFE, 0xE5, 0xE4,
    0x38, 0x77, 0x7C, 0xB6, 0xA9, 0x32, 0xDF, 0x8C, 0xD8, 0xBE, 0xC4, 0xD0,
    0x73, 0xB9, 0x31, 0xBA, 0x3B, 0xC8, 0x32, 0xB6, 0x8D, 0x9D, 0xD3, 0x00,
    0x74, 0x1F, 0xA7, 0xBF, 0x8A, 0xFC, 0x47, 0xED, 0x25, 0x76, 0xF6, 0x93,
    0x6B, 0xA4, 0x24, 0x66, 0x3A, 0xAB, 0x63, 0x9C, 0x5A, 0xE4, 0xF5, 0x68,
    0x34, 0x23, 0xB4, 0x74, 0x2B, 0xF1, 0xC9, 0x78, 0x23, 0x8F, 0x16, 0xCB,
    0xE3, 0x9D, 0x65, 0x2D, 0xE3, 0xFD, 0xB8, 0xBE, 0xFC, 0x84, 0x8A, 0xD9,
    0x22, 0x22, 0x2E, 0x04, 0xA4, 0x03, 0x7C, 0x07, 0x13, 0xEB, 0x57, 0xA8,
    0x1A, 0x23, 0xF0, 0xC7, 0x34, 0x73, 0xFC, 0x64, 0x6C, 0xEA, 0x30, 0x6B,
    0x4B, 0xCB, 0xC8, 0x86, 0x2F, 0x83, 0x85, 0xDD, 0xFA, 0x9D, 0x4B, 0x7F,
    0xA2, 0xC0, 0x87, 0xE8, 0x79, 0x68, 0x33, 0x03, 0xED, 0x5B, 0xDD, 0x3A,
    0x06, 0x2B, 0x3C, 0xF5, 0xB3, 0xA2, 0x78, 0xA6, 0x6D, 0x2A, 0x13, 0xF8,
    0x3F, 0x44, 0xF8, 0x2D, 0xDF, 0x31, 0x0E, 0xE0, 0x74, 0xAB, 0x6A, 0x36,
    0x45, 0x97, 0xE8, 0x99, 0xA0, 0x25, 0x5D, 0xC1, 0x64, 0xF3, 0x1C, 0xC5,
    0x08, 0x46, 0x85, 0x1D, 0xF9, 0xAB, 0x48, 0x19, 0x5D, 0xED, 0x7E, 0xA1,
    0xB1, 0xD5, 0x10, 0xBD, 0x7E, 0xE7, 0x4D, 0x73, 0xFA, 0xF3, 0x6B, 0xC3,
    0x1E, 0xCF, 0xA2, 0x68, 0x35, 0x90, 0x46, 0xF4, 0xEB, 0x87, 0x9F, 0x92,
    0x40, 0x09, 0x43, 0x8B, 0x48, 0x1C, 0x6C, 0xD7, 0x88, 0x9A, 0x00, 0x2E,
    0xD5, 0xEE, 0x38, 0x2B, 0xC9, 0x19, 0x0D, 0xA6, 0xFC, 0x02, 0x6E, 0x47,
    0x95, 0x58, 0xE4, 0x47, 0x56, 0x77, 0xE9, 0xAA, 0x9E, 0x30, 0x50, 0xE2,
    0x76, 0x56, 0x94, 0xDF, 0xC8, 0x1F, 0x56, 0xE8, 0x80, 0xB9, 0x6E, 0x71,
    0x60, 0xC9, 0x80, 0xDD, 0x98, 0xED, 0xD3, 0xDF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF};

#define P_GROUP18_LEN 1024 /* Size in bytes of the p number for group 18 */

bignum ssh_dh_generator;
bignum ssh_dh_group1;
bignum ssh_dh_group14;
bignum ssh_dh_group16;
bignum ssh_dh_group18;
static int dh_crypto_initialized;

/**
 * @internal
 * @brief Initialize global constants used in DH key agreement
 * @return SSH_OK on success, SSH_ERROR otherwise.
 */
int ssh_dh_init(void)
{
    unsigned long g_int = 2 ;	/* G is defined as 2 by the ssh2 standards */
    int rc;
    if (dh_crypto_initialized) {
        return SSH_OK;
    }
    dh_crypto_initialized = 1;

    ssh_dh_generator = bignum_new();
    if (ssh_dh_generator == NULL) {
        goto error;
    }
    rc = bignum_set_word(ssh_dh_generator, g_int);
    if (rc != 1) {
        goto error;
    }

    bignum_bin2bn(p_group1_value, P_GROUP1_LEN, &ssh_dh_group1);
    if (ssh_dh_group1 == NULL) {
        goto error;
    }
    bignum_bin2bn(p_group14_value, P_GROUP14_LEN, &ssh_dh_group14);
    if (ssh_dh_group14 == NULL) {
        goto error;
    }
    bignum_bin2bn(p_group16_value, P_GROUP16_LEN, &ssh_dh_group16);
    if (ssh_dh_group16 == NULL) {
        goto error;
    }
    bignum_bin2bn(p_group18_value, P_GROUP18_LEN, &ssh_dh_group18);
    if (ssh_dh_group18 == NULL) {
        goto error;
    }

    return 0;
error:
    ssh_dh_finalize();
    return SSH_ERROR;
}

/**
 * @internal
 * @brief Finalize and free global constants used in DH key agreement
 */
void ssh_dh_finalize(void)
{
    if (!dh_crypto_initialized) {
        return;
    }

    bignum_safe_free(ssh_dh_generator);
    bignum_safe_free(ssh_dh_group1);
    bignum_safe_free(ssh_dh_group14);
    bignum_safe_free(ssh_dh_group16);
    bignum_safe_free(ssh_dh_group18);

    dh_crypto_initialized = 0;
}

int ssh_dh_import_next_pubkey_blob(ssh_session session, ssh_string pubkey_blob)
{
    return ssh_pki_import_pubkey_blob(pubkey_blob,
                                      &session->next_crypto->server_pubkey);

}

static SSH_PACKET_CALLBACK(ssh_packet_client_dh_reply);

static ssh_packet_callback dh_client_callbacks[]= {
    ssh_packet_client_dh_reply
};

static struct ssh_packet_callbacks_struct ssh_dh_client_callbacks = {
    .start = SSH2_MSG_KEXDH_REPLY,
    .n_callbacks = 1,
    .callbacks = dh_client_callbacks,
    .user = NULL
};

/** @internal
 * @brief Starts diffie-hellman-group1 key exchange
 */
int ssh_client_dh_init(ssh_session session){
  struct ssh_crypto_struct *crypto = session->next_crypto;
#if !defined(HAVE_LIBCRYPTO) || OPENSSL_VERSION_NUMBER < 0x30000000L
  const_bignum pubkey;
#else
  bignum pubkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER */
  int rc;

  rc = ssh_dh_init_common(crypto);
  if (rc == SSH_ERROR) {
    goto error;
  }

  rc = ssh_dh_keypair_gen_keys(crypto->dh_ctx, DH_CLIENT_KEYPAIR);
  if (rc == SSH_ERROR){
      goto error;
  }
  rc = ssh_dh_keypair_get_keys(crypto->dh_ctx, DH_CLIENT_KEYPAIR,
                               NULL, &pubkey);
  if (rc != SSH_OK) {
    goto error;
  }
  rc = ssh_buffer_pack(session->out_buffer, "bB", SSH2_MSG_KEXDH_INIT, pubkey);
  if (rc != SSH_OK) {
    goto error;
  }
#if defined(HAVE_LIBCRYPTO) && OPENSSL_VERSION_NUMBER >= 0x30000000L
  bignum_safe_free(pubkey);
#endif

  /* register the packet callbacks */
  ssh_packet_set_callbacks(session, &ssh_dh_client_callbacks);
  session->dh_handshake_state = DH_STATE_INIT_SENT;

  rc = ssh_packet_send(session);
  return rc;
error:
#if defined(HAVE_LIBCRYPTO) && OPENSSL_VERSION_NUMBER >= 0x30000000L
  bignum_safe_free(pubkey);
#endif
  ssh_dh_cleanup(crypto);
  return SSH_ERROR;
}

SSH_PACKET_CALLBACK(ssh_packet_client_dh_reply){
  struct ssh_crypto_struct *crypto=session->next_crypto;
  ssh_string pubkey_blob = NULL;
  bignum server_pubkey;
  int rc;

  (void)type;
  (void)user;

  ssh_packet_remove_callbacks(session, &ssh_dh_client_callbacks);

  rc = ssh_buffer_unpack(packet, "SBS", &pubkey_blob, &server_pubkey,
          &crypto->dh_server_signature);
  if (rc == SSH_ERROR) {
      goto error;
  }
  rc = ssh_dh_keypair_set_keys(crypto->dh_ctx, DH_SERVER_KEYPAIR,
                               NULL, server_pubkey);
  if (rc != SSH_OK) {
      SSH_STRING_FREE(pubkey_blob);
      bignum_safe_free(server_pubkey);
      goto error;
  }
  rc = ssh_dh_import_next_pubkey_blob(session, pubkey_blob);
  SSH_STRING_FREE(pubkey_blob);
  if (rc != 0) {
      goto error;
  }

  rc = ssh_dh_compute_shared_secret(session->next_crypto->dh_ctx,
                                    DH_CLIENT_KEYPAIR, DH_SERVER_KEYPAIR,
                                    &session->next_crypto->shared_secret);
  ssh_dh_debug_crypto(session->next_crypto);
  if (rc == SSH_ERROR){
    ssh_set_error(session, SSH_FATAL, "Could not generate shared secret");
    goto error;
  }

  /* Send the MSG_NEWKEYS */
  if (ssh_buffer_add_u8(session->out_buffer, SSH2_MSG_NEWKEYS) < 0) {
    goto error;
  }

  rc=ssh_packet_send(session);
  if (rc == SSH_ERROR) {
    goto error;
  }

  SSH_LOG(SSH_LOG_PROTOCOL, "SSH_MSG_NEWKEYS sent");
  session->dh_handshake_state = DH_STATE_NEWKEYS_SENT;
  return SSH_PACKET_USED;
error:
  ssh_dh_cleanup(session->next_crypto);
  session->session_state=SSH_SESSION_STATE_ERROR;
  return SSH_PACKET_USED;
}

#ifdef WITH_SERVER

static SSH_PACKET_CALLBACK(ssh_packet_server_dh_init);

static ssh_packet_callback dh_server_callbacks[] = {
    ssh_packet_server_dh_init,
};

static struct ssh_packet_callbacks_struct ssh_dh_server_callbacks = {
    .start = SSH2_MSG_KEXDH_INIT,
    .n_callbacks = 1,
    .callbacks = dh_server_callbacks,
    .user = NULL
};

/** @internal
 * @brief sets up the diffie-hellman-groupx kex callbacks
 */
void ssh_server_dh_init(ssh_session session){
    /* register the packet callbacks */
    ssh_packet_set_callbacks(session, &ssh_dh_server_callbacks);

    ssh_dh_init_common(session->next_crypto);
}

/** @internal
 * @brief processes a SSH_MSG_KEXDH_INIT or SSH_MSG_KEX_DH_GEX_INIT packet and sends
 * the appropriate SSH_MSG_KEXDH_REPLY or SSH_MSG_KEX_DH_GEX_REPLY
 */
int ssh_server_dh_process_init(ssh_session session, ssh_buffer packet)
{
    struct ssh_crypto_struct *crypto = session->next_crypto;
    ssh_key privkey = NULL;
    enum ssh_digest_e digest = SSH_DIGEST_AUTO;
    ssh_string sig_blob = NULL;
    ssh_string pubkey_blob = NULL;
    bignum client_pubkey;
#if !defined(HAVE_LIBCRYPTO) || OPENSSL_VERSION_NUMBER < 0x30000000L
    const_bignum server_pubkey;
#else
    bignum server_pubkey = NULL;
#endif /* OPENSSL_VERSION_NUMBER */
    int packet_type;
    int rc;

    rc = ssh_buffer_unpack(packet, "B", &client_pubkey);
    if (rc == SSH_ERROR) {
        ssh_set_error(session, SSH_FATAL, "No e number in client request");
        goto error;
    }

    rc = ssh_dh_keypair_set_keys(crypto->dh_ctx, DH_CLIENT_KEYPAIR,
                                 NULL, client_pubkey);
    if (rc != SSH_OK) {
        bignum_safe_free(client_pubkey);
        goto error;
    }

    rc = ssh_dh_keypair_gen_keys(crypto->dh_ctx, DH_SERVER_KEYPAIR);
    if (rc == SSH_ERROR) {
        goto error;
    }

    rc = ssh_get_key_params(session, &privkey, &digest);
    if (rc != SSH_OK) {
        goto error;
    }
    rc = ssh_dh_compute_shared_secret(crypto->dh_ctx,
                                      DH_SERVER_KEYPAIR, DH_CLIENT_KEYPAIR,
                                      &crypto->shared_secret);
    ssh_dh_debug_crypto(crypto);
    if (rc == SSH_ERROR) {
        ssh_set_error(session, SSH_FATAL, "Could not generate shared secret");
        goto error;
    }
    rc = ssh_make_sessionid(session);
    if (rc != SSH_OK) {
        ssh_set_error(session, SSH_FATAL, "Could not create a session id");
        goto error;
    }
    sig_blob = ssh_srv_pki_do_sign_sessionid(session, privkey, digest);
    if (sig_blob == NULL) {
        ssh_set_error(session, SSH_FATAL, "Could not sign the session id");
        goto error;
    }
    switch (crypto->kex_type){
    case SSH_KEX_DH_GROUP1_SHA1:
    case SSH_KEX_DH_GROUP14_SHA1:
    case SSH_KEX_DH_GROUP14_SHA256:
    case SSH_KEX_DH_GROUP16_SHA512:
    case SSH_KEX_DH_GROUP18_SHA512:
        packet_type = SSH2_MSG_KEXDH_REPLY;
        break;
#ifdef WITH_GEX
    case SSH_KEX_DH_GEX_SHA1:
    case SSH_KEX_DH_GEX_SHA256:
        packet_type = SSH2_MSG_KEX_DH_GEX_REPLY;
        break;
#endif /* WITH_GEX */
    default:
        ssh_set_error(session, SSH_FATAL, "Invalid kex type");
        goto error;
    }
    rc = ssh_dh_keypair_get_keys(crypto->dh_ctx, DH_SERVER_KEYPAIR,
                                 NULL, &server_pubkey);
    if (rc != SSH_OK){
        goto error;
    }
    rc = ssh_dh_get_next_server_publickey_blob(session, &pubkey_blob);
    if (rc != SSH_OK){
        ssh_set_error_oom(session);
        goto error;
    }
    rc = ssh_buffer_pack(session->out_buffer,
                         "bSBS",
                         packet_type,
                         pubkey_blob,
                         server_pubkey,
                         sig_blob);
    SSH_STRING_FREE(sig_blob);
    SSH_STRING_FREE(pubkey_blob);
#if defined(HAVE_LIBCRYPTO) && OPENSSL_VERSION_NUMBER >= 0x30000000L
    bignum_safe_free(server_pubkey);
#endif
    if(rc != SSH_OK) {
        ssh_set_error_oom(session);
        ssh_buffer_reinit(session->out_buffer);
        goto error;
    }
    rc = ssh_packet_send(session);
    if (rc == SSH_ERROR) {
        goto error;
    }
    SSH_LOG(SSH_LOG_DEBUG, "Sent KEX_DH_[GEX]_REPLY");

    if (ssh_buffer_add_u8(session->out_buffer, SSH2_MSG_NEWKEYS) < 0) {
        ssh_buffer_reinit(session->out_buffer);
        goto error;
    }
    session->dh_handshake_state=DH_STATE_NEWKEYS_SENT;
    if (ssh_packet_send(session) == SSH_ERROR) {
        goto error;
    }
    SSH_LOG(SSH_LOG_PACKET, "SSH_MSG_NEWKEYS sent");

    return SSH_OK;
error:
    SSH_STRING_FREE(sig_blob);
    SSH_STRING_FREE(pubkey_blob);
#if defined(HAVE_LIBCRYPTO) && OPENSSL_VERSION_NUMBER >= 0x30000000L
    bignum_safe_free(server_pubkey);
#endif

    session->session_state = SSH_SESSION_STATE_ERROR;
    ssh_dh_cleanup(session->next_crypto);
    return SSH_ERROR;
}

/** @internal
 * @brief parse an incoming SSH_MSG_KEXDH_INIT packet and complete
 *        Diffie-Hellman key exchange
 **/
static SSH_PACKET_CALLBACK(ssh_packet_server_dh_init){
    (void)type;
    (void)user;
    SSH_LOG(SSH_LOG_DEBUG, "Received SSH_MSG_KEXDH_INIT");
    ssh_packet_remove_callbacks(session, &ssh_dh_server_callbacks);
    ssh_server_dh_process_init(session, packet);
    return SSH_PACKET_USED;
}

/** @internal
 * @brief Choose a fallback group for the DH Group exchange if the
 * moduli file is not readable
 * @param[in] pmax maximum requestsd group size
 * @param[out] modulus
 * @param[out] generator
 * @returns SSH_OK on success, SSH_ERROR otherwise
 */
int ssh_fallback_group(uint32_t pmax,
                       bignum *modulus,
                       bignum *generator)
{
    *modulus = NULL;
    *generator = NULL;

    if (pmax < 3072) {
        bignum_dup(ssh_dh_group14, modulus);
    } else if (pmax < 6144) {
        bignum_dup(ssh_dh_group16, modulus);
    } else {
        bignum_dup(ssh_dh_group18, modulus);
    }
    if (*modulus == NULL) {
        return SSH_ERROR;
    }

    bignum_dup(ssh_dh_generator, generator);
    if (*generator == NULL) {
        bignum_safe_free((*modulus));
        return SSH_ERROR;
    }

    return SSH_OK;
}

#endif /* WITH_SERVER */

/**
 * @addtogroup libssh_session
 *
 * @{
 */

bool ssh_dh_is_known_group(bignum modulus, bignum generator)
{
    int cmp, bits;
    bignum m = NULL;

    bits = bignum_num_bits(modulus);
    if (bits < 3072) {
        m = ssh_dh_group14;
    } else if (bits < 6144) {
        m = ssh_dh_group16;
    } else {
        m = ssh_dh_group18;
    }

    cmp = bignum_cmp(m, modulus);
    if (cmp != 0) {
        return false;
    }

    cmp = bignum_cmp(ssh_dh_generator, generator);
    if (cmp != 0) {
        return false;
    }

    SSH_LOG(SSH_LOG_TRACE, "The received primes in FIPS are known");
    return true;
}

ssh_key ssh_dh_get_current_server_publickey(ssh_session session)
{
    if (session->current_crypto == NULL) {
        return NULL;
    }

    return session->current_crypto->server_pubkey;
}

/* Caller needs to free the blob */
int ssh_dh_get_current_server_publickey_blob(ssh_session session,
                                     ssh_string *pubkey_blob)
{
    const ssh_key pubkey = ssh_dh_get_current_server_publickey(session);

    return ssh_pki_export_pubkey_blob(pubkey, pubkey_blob);
}

ssh_key ssh_dh_get_next_server_publickey(ssh_session session)
{
    return session->next_crypto->server_pubkey;
}

/* Caller needs to free the blob */
int ssh_dh_get_next_server_publickey_blob(ssh_session session,
                                          ssh_string *pubkey_blob)
{
    const ssh_key pubkey = ssh_dh_get_next_server_publickey(session);

    return ssh_pki_export_pubkey_blob(pubkey, pubkey_blob);
}

/**
 * @internal
 *
 * @brief Convert a buffer into an unpadded base64 string.
 * The caller has to free the memory.
 *
 * @param  hash         What should be converted to a base64 string.
 *
 * @param  len          Length of the buffer to convert.
 *
 * @return              The base64 string or NULL on error.
 *
 * @see ssh_string_free_char()
 */
static char *ssh_get_b64_unpadded(const unsigned char *hash, size_t len)
{
    char *b64_padded = NULL;
    char *b64_unpadded = NULL;
    size_t k;

    b64_padded = (char *)bin_to_base64(hash, len);
    if (b64_padded == NULL) {
        return NULL;
    }
    for (k = strlen(b64_padded); k != 0 && b64_padded[k-1] == '='; k--);

    b64_unpadded = strndup(b64_padded, k);
    SAFE_FREE(b64_padded);

    return b64_unpadded;
}

/**
 * @brief Get a hash as a human-readable hex- or base64-string.
 *
 * This gets an allocated fingerprint hash.  If it is a SHA sum, it will
 * return an unpadded base64 string.  If it is a MD5 sum, it will return a hex
 * string. Either way, the output is prepended by the hash-type.
 *
 * @warning Do NOT use MD5 or SHA1! Those hash functions are being deprecated.
 *
 * @param  type         Which sort of hash is given, use
 *                      SSH_PUBLICKEY_HASH_SHA256 or better.
 *
 * @param  hash         The hash to be converted to fingerprint.
 *
 * @param  len          Length of the buffer to convert.
 *
 * @return Returns the allocated fingerprint hash or NULL on error. The caller
 *         needs to free the memory using ssh_string_free_char().
 *
 * @see ssh_string_free_char()
 */
char *ssh_get_fingerprint_hash(enum ssh_publickey_hash_type type,
                               unsigned char *hash,
                               size_t len)
{
    const char *prefix = "UNKNOWN";
    char *fingerprint = NULL;
    char *str = NULL;
    size_t str_len;
    int rc;

    switch (type) {
    case SSH_PUBLICKEY_HASH_SHA1:
    case SSH_PUBLICKEY_HASH_SHA256:
        fingerprint = ssh_get_b64_unpadded(hash, len);
        break;
    case SSH_PUBLICKEY_HASH_MD5:
        fingerprint = ssh_get_hexa(hash, len);
        break;
    }
    if (fingerprint == NULL) {
        return NULL;
    }

    switch (type) {
    case SSH_PUBLICKEY_HASH_MD5:
        prefix = "MD5";
        break;
    case SSH_PUBLICKEY_HASH_SHA1:
        prefix = "SHA1";
        break;
    case SSH_PUBLICKEY_HASH_SHA256:
        prefix = "SHA256";
        break;
    }

    str_len = strlen(prefix);
    if (str_len + 1 + strlen(fingerprint) + 1 < str_len) {
        SAFE_FREE(fingerprint);
        return NULL;
    }
    str_len += 1 + strlen(fingerprint) + 1;

    str = malloc(str_len);
    if (str == NULL) {
        SAFE_FREE(fingerprint);
        return NULL;
    }
    rc = snprintf(str, str_len, "%s:%s", prefix, fingerprint);
    SAFE_FREE(fingerprint);
    if (rc < 0 || rc < (int)(str_len - 1)) {
        SAFE_FREE(str);
    }

    return str;
}

/**
 * @brief Print a hash as a human-readable hex- or base64-string.
 *
 * This prints an unpadded base64 strings for SHA sums and hex strings for MD5
 * sum.  Either way, the output is prepended by the hash-type.
 *
 * @param  type         Which sort of hash is given. Use
 *                      SSH_PUBLICKEY_HASH_SHA256 or better.
 *
 * @param  hash         The hash to be converted to fingerprint.
 *
 * @param  len          Length of the buffer to convert.
 *
 * @see ssh_get_publickey_hash()
 * @see ssh_get_fingerprint_hash()
 */
void ssh_print_hash(enum ssh_publickey_hash_type type,
                    unsigned char *hash,
                    size_t len)
{
    char *fingerprint = NULL;

    fingerprint = ssh_get_fingerprint_hash(type,
                                           hash,
                                           len);
    if (fingerprint == NULL) {
        return;
    }

    fprintf(stderr, "%s\n", fingerprint);

    SAFE_FREE(fingerprint);
}

/** @} */
