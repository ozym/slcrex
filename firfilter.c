/*
 * Copyright (c) 2003 Institute of Geological & Nuclear Sciences Ltd.
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

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fnmatch.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <netdb.h>
#include <time.h>
#include <math.h>

/* local includes */
#include "firfilter.h"

static int nfirs = 0;
static fir_t firs[FIR_MAX_FILTERS];

/* load in a set of FIR filters ... */
int firfilter_load(char *fname) {

	int c, n, m;
	char str[FIR_LEN_LINE];

	char *s = NULL;
	FILE *fp = NULL; /* file pointer */

    int nf = 0;
	fir_t *f; /* pointer to actual filter ... */

	/* clear the decks ... */
    nfirs = 0;
	memset(firs, 0, sizeof(firs));

	/* try and open the fir file ... */
	if ((fp = fopen(fname, "r")) == NULL) {
        return -1;
	}

	/* first line is the number of filters ... */
	if ((!fgets(str, sizeof(str), fp)) || (sscanf(str, "%d", &nf) != 1)) {
		fclose(fp); return -1;
	}

	/* make running space .... */
	for (n = 0, f = firs; (n < nf); n++, f++) {
		if ((!fgets(str, sizeof(str), fp)) || (sscanf(str, "%s", (char *) &f->label) != 1)) {
			fclose(fp); return -1;
		}
		if ((!fgets(str, sizeof(str), fp)) || (sscanf(str, "%d", &f->length) != 1)) {
			fclose(fp); return -1;
		}
		if ((!fgets(str, sizeof(str), fp)) || (sscanf(str, "%lf", &f->gain) != 1)) {
			fclose(fp); return -1;
		}
		if ((!fgets(str, sizeof(str), fp)) || (sscanf(str, "%d", &f->decimate) != 1)) {
			fclose(fp); return -1;
		}

		/* is it a minimum phase filter? ... */
		f->minimum = ((toupper(f->label[strlen(f->label) - 1]) == 'M') ? 1 : 0);

		/* how many sample points to read */
		if ((c = (f->minimum ? f->length : f->length / 2)) > FIR_MAX_POINTS) {
			fclose(fp); return -1;
		}

		f->gain = 0.0;

		m = 0;
		while (m < c) {

			/* load in the line ... */
			if (!fgets(str, sizeof(str), fp)) {
				fclose(fp); return -1;
			}

			/* perhaps remove the following cr */
			if (str[strlen(str) - 1] == '\n') {
				str[strlen(str) - 1] = '\0';
			}

			/* now decode the FIR points */
			s = strtok(str, " \t\n");
			while((m < c) && (s != NULL)) {
				f->gain += ((f->minimum) ? 1.0 : 2.0) * atof(s);
				f->points[m++] = atof(s);
				s = strtok(NULL, " \t\n");
			}
		}
	}

	(void) fclose(fp);

	/* answer */
	nfirs = ((nf < 0) ? 0 : nf);

	return nfirs;
}

int firfilter_find(char *label, fir_t *fir) {
	int n;

	for (n = 0; n < nfirs; n++) {
		if (strncasecmp(firs[n].label, label, FIR_LEN_LABEL) == 0) break;
	}

	if (n < nfirs)
		memcpy(fir, &firs[n], sizeof(fir_t));

	return ((n < nfirs) ?  0 : -1);
}

static double firfilter_value(fir_t *f) {
	int i, j, n; /* counters */
	double v = 0.0; /* return value */

	/* minimum phase */
	if (f->minimum) {
		for (i = 0; i < f->length; i++) {
			j = (f->offset + i) % f->length;
			v += (f->samples[j] * f->points[i]);
		}
	}
	else {
		n = f->length / 2;
		for (i = 0; i < n; i++) {
			j = (f->offset + i) % f->length;
			v += (f->samples[j] * f->points[i]);
		}
		for (i = 0; i < n; i++) {
			j = (f->offset + n + i) % f->length;
			v += (f->samples[j] * f->points[n - i - 1]);
		}
	}

	/* adjust the output gain */
	v /= f->gain;

	return(v);
}

void firfilter_reset(fir_t *f) {
	f->count = f->offset = 0;
}

int firfilter_apply(fir_t *f, double *v) {
	/* apply a FIR filter .... */

	/* we need to add the new sample to the ring */
	f->samples[f->offset] = (double)(*v);
	f->offset = (f->offset + 1) % f->length;

	/* have we a full set ... */
	if ((++f->count) == f->length) {

		/* apply the filter .... */
		(*v) = firfilter_value(f);

		/* processed a sample ... */
		f->count -= f->decimate;

		return(1);
	}

	return(0);
}
