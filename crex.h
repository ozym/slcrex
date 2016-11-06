#ifndef _CREX_H_
#define _CREX_H_

#ifndef GAUSSIAN_BUFLEN
#define GAUSSIAN_BUFLEN 120
#endif /* GAUSSIAN_BUFLEN */

#ifndef GAUSSIAN_DELAY
#define GAUSSIAN_DELAY 45
#endif /* GAUSSIAN_DELAY */

#ifndef GAUSSIAN_OFFSET
#define GAUSSIAN_OFFSET 60
#endif /* GAUSSIAN_OFFSET */

#ifndef CREX_BUF_SIZE
#define CREX_BUF_SIZE 6
#endif /* CREX_BUF_SIZE */

#ifndef CREX_NO_DATA
#define CREX_NO_DATA INT_MIN
#endif /* CREX_NO_DATA */

#ifndef CREX_HDR_STR
#define CREX_HDR_STR "CREX++\nT000103 A001 D01021 D06025++"
#endif /* CREX_HDR_STR */

/* time representation in CREX messages */
typedef struct _crex_time {
    int year;           /* Year, width=4 */
    short int month;        /* Month, width=2 */
    short int day;      /* Day, width=2 */
    short int hour;     /* Hour, width=2 */
    short int minute;       /* Minute, width=2 */
} crex_time_t;

typedef struct _crex_tidal {
    int num_tides;
    double zone;
    double latitude;
    tidal_t tides[LIBTIDAL_MAX_CONSTITUENTS];
} crex_tidal_t;

/* used for storing CREX message data */
typedef struct _crex_tide_data {
    char id[25];            /* ID string to store lat, lon and id. */
    time_t time;
    int temp;               /* Sea/water temperature, unit=K, scale=1, width=4; default = -1;*/
    short int autoQC;       /* Tide station automated water level check, unit=code table, width=2; default = 11 */
    short int manualQC;     /* Tide station manual water level check, unit=code table, width=2; default = 7 */
    short int offset;       /* Time increment [offset], unit=Minute, scale=0, width=4 (NOTE: Change [next] data width to 2 characters); default = 0 */
    short int increment;    /* Short time increment, unit=Minute, scale=0, width=2; default = 1 */
    int mes[CREX_BUF_SIZE]; /* Tidal elevation with respect to local chart datum, unit=m, scale=3, width=5 */
    int res[CREX_BUF_SIZE]; /* Meteorological res tidal elevation (surge or offset), unit=m, scale=3, width=4 */
} crex_tide_data_t;

typedef struct _crex_stream {
    char srcname[100];

    double alpha;
    double beta;

    int nfirs;
    fir_t firs[FIR_MAX_FILTERS];

    int under;
    double samprate;
    hptime_t delay;
    hptime_t endtime;

    int count; /* length of obs buffer */
    int offset; /* offset into obs buffer */
    double obs[GAUSSIAN_BUFLEN]; /* 1 sps values */
    crex_tide_data_t ctd; /* crex coded data */

    time_t next_obs; /* expected next sample time */

    struct _crex_stream *next;
} crex_stream_t;

static double GAUSSIAN[GAUSSIAN_DELAY + 1] = {
    0.02519580, 0.02514602, 0.02499727, 0.02475132, 0.02441104, 0.02398040,
    0.02346437, 0.02286881, 0.02220039, 0.02146643, 0.02067480, 0.01983377,
    0.01895183, 0.01803763, 0.01709976, 0.01614667, 0.01518651, 0.01422707,
    0.01327563, 0.01233892, 0.01142303, 0.01053338, 0.00967467, 0.00885090,
    0.00806530, 0.00732042, 0.00661811, 0.00595955, 0.00534535, 0.00477552,
    0.00424959, 0.00376666, 0.00332543, 0.00292430, 0.00256140, 0.00223468,
    0.00194194, 0.00168089, 0.00144918, 0.00124449, 0.00106449, 0.00090693,
    0.00076964, 0.00065055, 0.00054772, 0.00045933
};

int process_crex(MSRecord *msr, crex_tidal_t *tides, crex_stream_t *stream, void (*record_handler) (char *, int, void *), void *handlerdata, int *packedsamples, double timetol, int verbose);

#endif // _CREX_H_
