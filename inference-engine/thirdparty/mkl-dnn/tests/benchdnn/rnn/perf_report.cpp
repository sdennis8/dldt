/*******************************************************************************
 * Copyright 2018 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mkldnn.h"
#include "mkldnn_memory.hpp"

#include "rnn/rnn.hpp"

namespace rnn {

void perf_report(const rnn_prb_t *p, const res_t *r, const char *pstr) {
    const auto &t = r->timer;
    const int max_len = 400;
    int rem_len = max_len - 1;
    char buffer[max_len], *buf = buffer;

#   define DPRINT(...) do { \
        int l = snprintf(buf, rem_len, __VA_ARGS__); \
        buf += l; rem_len -= l; \
    } while(0)

    auto modifier2mode = [](char c) {
        if (c == '-') return benchdnn_timer_t::min;
        if (c == '0') return benchdnn_timer_t::avg;
        if (c == '+') return benchdnn_timer_t::max;
        return benchdnn_timer_t::min;
    };

    auto modifier2unit = [](char c) {
        if (c == 'K') return 1e3;
        if (c == 'M') return 1e6;
        if (c == 'G') return 1e9;
        return 1e0;
    };

    const char *pt = perf_template;
    char c;

    while ((c = *pt++) != '\0') {
        if (c != '%') { *buf++ = c; rem_len--; continue; }

        c = *pt++;

        benchdnn_timer_t::mode_t mode = benchdnn_timer_t::min;
        double unit = 1e0;

        if (c == '-' || c == '0' || c == '+') {
            mode = modifier2mode(c);
            c = *pt++;
        }

        if (c == 'K' || c == 'M' || c == 'G') {
            unit = modifier2unit(c);
            c = *pt++;
        }
        // cellkind:activation:direction:l d mb
        if (c == 'd') DPRINT("%s_%s_%s_l%dd%dt%dmb%d_slc%dsic%ddic%d",
                             alg2str(p->alg), activation2str(p->activation), direction2str(p->direction),
                             p->n_layer, p->n_directions(), p->n_iter, p->mb, p->slc, p->sic, p->dic);
        else if (c == 'D')
            DPRINT("%s", pstr);
        else if (c == 'n')
            DPRINT("%s", p->name);
        else if (c == 'z')
            DPRINT("%s", prop2str(p->prop));
        else if (c == 'F')
            DPRINT("%g", t.ticks(mode) / t.ms(mode) / unit * 1e3);
        else if (c == 't')
            DPRINT("%g", t.ms(mode) / unit);
        else if (c == 'c')
            DPRINT("%g", t.ticks(mode) / unit);
        else
            []() { SAFE(FAIL, CRIT); return 0; }();
    }

    *buf = '\0';
    assert(rem_len >= 0);

#   undef DPRINT
    print(0, "%s\n", buffer);
}

} // namespace rnn
