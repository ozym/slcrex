#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <math.h>

#include "libmseed.h"
#include "libtidal.h"
#include "firfilter.h"
#include "crex.h"

/* generate a crex message string */
static int generate_crex_str(char *crex_str, crex_tide_data_t *ctd) {
    int i, j;
    int mon, mday;
    BTime btime;

    /* Because of funny forward incrementing CREX time-stamps, the message
     * time-stamp is actually one minute before the *oldest* sample and residual. */
    if ((ms_hptime2btime (MS_EPOCH2HPTIME((ctd->time - CREX_BUF_SIZE * 60)), &btime)) < 0) 
        return -1;

    ms_doy2md(btime.year, btime.day, &mon, &mday);

    i = sprintf(crex_str, "%-23s %04d %02d %02d %02d %02d //// %02d %02d %02d %02d\n",
            ctd->id, btime.year, mon, mday, btime.hour, btime.min,
            ctd->autoQC, ctd->manualQC, ctd->offset, ctd->increment);

    /* The most recent sample pair appear on the *right*. */
    for (j = CREX_BUF_SIZE - 1; j >= 0; j--) {
        if (ctd->mes[j] == CREX_NO_DATA) {
            i += sprintf(crex_str + i, "///// //// ");
        } else {
            i += sprintf(crex_str + i, "%.5d ", ctd->mes[j]);
            if (ctd->res[j] == CREX_NO_DATA) {
                i += sprintf(crex_str + i, "//// ");
            } else {
                i += sprintf(crex_str + i, "%.4d ", ctd->res[j]);
            }
        }
    }

    crex_str[i-1] = '+';
    crex_str[i] = '\0';

    return (i);
}

int process_crex(MSRecord *msr, crex_tidal_t *tides, crex_stream_t *stream, void (*record_handler) (char *, int, void *), void *handlerdata, int *packedsamples, double timetol, int verbose) {
    int i, j;
    int n, m;

    time_t start = 0;
    time_t seconds = 0;

    char samples[512];
    char isotime[256];

    int offset = 0;
    double obs = 0.0;
    double height = 0.0;

    long psamples = 0;
    long numsamples = 0;
    MSRecord *xmsr = NULL;

    double value = 0.0;
    double hpdelta = 0.0;
    hptime_t hptimetol = 0LL;

    /* need at least a sample */
    if (msr->samplecnt < 1)
        return 0;

    /* we need an integer value */
    if (msr->sampletype != 'i')
        return 0;

    /* problem with rate */
    if (msr->samprate == 0.0)
        return 0;

    if ( ! MS_ISRATETOLERABLE(1.0, stream->samprate)) {
        ms_log(1, "invalid sample rate: %g\n", stream->samprate); return -1;
    }

    hpdelta = (msr->samprate ) ? (HPTMODULUS / msr->samprate) : 0.0;
    if ( timetol == -1.0 )
        hptimetol = 0.5 * hpdelta;
    else if ( timetol >= 0.0 )
        hptimetol = (hptime_t) (timetol * HPTMODULUS);

    if (fabs((double)(msr->starttime - stream->endtime) - hpdelta) > hptimetol) {
        stream->count = 0;
        stream->under = (int) ceil((double) (HPTMODULUS - (msr->starttime - stream->delay) % HPTMODULUS) / hpdelta);
        ms_log(0, "reset %s %lld (%d)\n", stream->srcname, ms_hptime2isotimestr(msr->starttime, isotime, 1), msr->starttime - stream->endtime, stream->under);
        for (m = 0; m < stream->nfirs; m++) {
            firfilter_reset(&stream->firs[m]);
        }
    }

    (*packedsamples) = 0;

    for (n = 0; n < msr->numsamples; n++) {
        if (stream->under-- > 0) continue;

        value = (double) ((int *) msr->datasamples)[n];
        for (m = 0; m < stream->nfirs; m++) {
            if (firfilter_apply(&stream->firs[m], &value) != 1) break;
        }

        /* need to have seen each filter */
        if (m < stream->nfirs) continue;

#ifdef FULL
            time_interval2(n, hdr->sample_rate, hdr->sample_rate_mult, &secs, &usecs);
            it = add_dtime(add_time(hdr->begtime, secs, usecs), delay);

            et = int_to_ext(it);
            lookup = step = (length > 0) ? step : et.second;
            /* TODO: VERIFY the bounds check on lookup unnecessary. (lookup < 60). */
            gaussian[lookup] = meters;
            step = ((step + 1) % GAUSSIAN_BUFLEN);
            length += ((length < GAUSSIAN_BUFLEN) ? 1 : 0);
#endif

        start = (time_t) rint((double) MS_HPTIME2EPOCH((msr->starttime + n * (hptime_t) hpdelta - stream->delay)));
        //fprintf(stderr, "%s\n", ms_hptime2isotimestr((msr->starttime + n * (hptime_t) hpdelta - stream->delay), isotime, 1));

        offset = (stream->count > 0) ? stream->offset : start % GAUSSIAN_BUFLEN;
        stream->offset = (stream->offset + 1) % GAUSSIAN_BUFLEN;

        //fprintf(stderr, "check sample %s %s %d count=%d\n", stream->srcname, ms_hptime2isotimestr(MS_EPOCH2HPTIME(start), isotime, 1), offset, stream->count);

        stream->obs[offset] = value;
        if (stream->count < GAUSSIAN_BUFLEN)
            stream->count++;
        if (stream->count < GAUSSIAN_BUFLEN)
            continue;
        if ((offset % GAUSSIAN_OFFSET) != GAUSSIAN_DELAY)
            continue;

        obs = stream->obs[(offset - GAUSSIAN_DELAY + GAUSSIAN_BUFLEN) % GAUSSIAN_BUFLEN] * GAUSSIAN[0];
        for (m = 1; m <= GAUSSIAN_DELAY; m++) {
            obs += stream->obs[(offset - GAUSSIAN_DELAY - m + GAUSSIAN_BUFLEN) % GAUSSIAN_BUFLEN] * GAUSSIAN[m];
            obs += stream->obs[(offset - GAUSSIAN_DELAY + m + GAUSSIAN_BUFLEN) % GAUSSIAN_BUFLEN] * GAUSSIAN[m];
        }

        fprintf(stderr, "add sample %s %s %d %g\n", stream->srcname, ms_hptime2isotimestr(MS_EPOCH2HPTIME(start), isotime, 1), offset, obs);

        seconds = (time_t) (start - GAUSSIAN_DELAY);
        if (seconds < stream->ctd.time)
            continue;

        libtidal_height(tides->num_tides, tides->tides, seconds, tides->latitude, tides->zone, &height);

        j = (seconds - stream->ctd.time) / 60;
        j = (j < CREX_BUF_SIZE) ? j : CREX_BUF_SIZE;

        /* Shift all the measurements and residuals by the number of missing CREX messages,
         * filling any missing values (other than the mes and res just calculated) with CREX_NO_DATA
         * and finally we insert the new values. */
        for (i = CREX_BUF_SIZE-1; i > j-1; i--) {
            stream->ctd.mes[i] = stream->ctd.mes[i-j];
            stream->ctd.res[i] = stream->ctd.res[i-j];
        }
        for (i = 1; i < j; i++) {
            stream->ctd.mes[i] = CREX_NO_DATA;
            stream->ctd.res[i] = CREX_NO_DATA;
        }

        /* Place the new values in the bottom. */
        stream->ctd.mes[0] = (int) rint(obs * stream->beta + stream->alpha);

        /* Sanity check the measurement */
        if (abs(stream->ctd.mes[0]) > 99999) {
            stream->ctd.mes[0] = CREX_NO_DATA;
            stream->ctd.res[0] = CREX_NO_DATA;
        }
        if (tides->num_tides > 0) {
            stream->ctd.res[0] = (int) rint(obs * stream->beta + stream->alpha - height);
            if (abs(stream->ctd.res[0]) > 9999) {
                stream->ctd.res[0] = CREX_NO_DATA;
            }
        }
        else {
            stream->ctd.res[0] = CREX_NO_DATA;
        }

        /* And reset the ctd time. */
        stream->ctd.time = seconds;

        if ((numsamples = generate_crex_str(samples, &stream->ctd)) < 0) {
            ms_log(1, "unable to generate crex string\n"); return -1;
        }

        if ((xmsr = msr_duplicate(msr, 0)) == NULL) {
            ms_log(1, "unable to duplicate msr block\n"); return -1;
        }
        strncpy(xmsr->channel, "CRX", 3);
        xmsr->starttime = MS_EPOCH2HPTIME((seconds));
        xmsr->sampletype = 'a';
        xmsr->numsamples = numsamples;
        xmsr->datasamples = (void *) malloc (numsamples);
        strncpy((char *) xmsr->datasamples, samples, numsamples);
        xmsr->samprate = 0;
        xmsr->encoding = 0;

        msr_normalize_header(xmsr, verbose);

        msr_print(xmsr, (verbose > 1) ? 1 : 0);
        msr_pack(xmsr, record_handler, handlerdata, &psamples, 1, (verbose > 1) ? 1 : 0);

        (*packedsamples) += psamples;

        msr_free(&xmsr);
    }
    stream->endtime = msr_endtime(msr);

    return 0;
}
