/* Copyright (c) 2003 Institute of Geological & Nuclear Sciences Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *		notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _FIRFILTERS_H_
#define _FIRFILTERS_H_

#define FIR_MAX_FILTERS 32
#define FIR_MAX_POINTS 2048 /* max points per FIR filter */
#define FIR_LEN_LINE 128
#define FIR_LEN_LABEL 32

struct _fir {
	char label[FIR_LEN_LABEL]; /* FIR filter label */
	int decimate; /* decimation factor */
	int minimum; /* minimum phase? */
	int length; /* number of points ... */
	int offset; /* sample buffer offset ... */
	int count; /* sample buffer count ... */
	double gain; /* gain of filter */
	double points[FIR_MAX_POINTS]; /* filter points */
	double samples[FIR_MAX_POINTS]; /* sample points */
};
typedef struct _fir fir_t;

int firfilter_load(char *fname);
int firfilter_find(char *label, fir_t *fir);
int firfilter_apply(fir_t *fir, double *value);
void firfilter_reset(fir_t *f);

#endif /* _FIRFILTERS_H_ */
