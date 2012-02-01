/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * This file is part of the EMHF historical reference
 * codebase, and is released under the terms of the
 * GNU General Public License (GPL) version 2.
 * Please see the LICENSE file for details.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

#include <emhf.h> 

#define COLS     80
#define ROWS     25
#define ATTR     7

static char *vidmem=(char *)0xB8000;
static unsigned int vid_x, vid_y;

static void vgamem_newln(void){
    vid_x = 0;
    vid_y++;

    if (vid_y >= ROWS){
        vid_y = ROWS-1;
        memcpy((char*)vidmem,(char*)vidmem + 2*COLS, (ROWS-1)*2*COLS);
        memcpy((char*)vidmem + (ROWS-1)*2*COLS, 0, 2*COLS);
    }
}

void dbg_x86_vgamem_putc(int c)
{
    if ( c == '\n' )
        vgamem_newln();
    else
    {
        vidmem[(vid_x + vid_y * COLS) * 2]  = c & 0xFF;
        vidmem[(vid_x + vid_y * COLS) * 2 + 1] = ATTR;
        if ( ++vid_x >= COLS ) vgamem_newln();
    }
}

void dbg_x86_vgamem_putstr(const char *str)
{
    int c;

    while ( (c = *str++) != '\0' )
        dbg_x86_vgamem_putc(c);
}


void dbg_x86_vgamem_init(char *params){
  (void)params;	//we don't use params for the VGA backend currently
  memset((char *)vidmem, 0, COLS * ROWS * 2);
  vid_x = vid_y = 0;
}
