/*
 * Nt time functions.
 *
 * RtlTimeToTimeFields, RtlTimeFieldsToTime and defines are taken from ReactOS and
 * adapted to wine with special permissions of the author. This code is
 * Copyright 2002 Rex Jolliff (rex@lvcablemodem.com)
 *
 * Copyright 1999 Juergen Schmied
 * Copyright 2007 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "ntdll.h"

typedef short CSHORT;

typedef struct _TIME_FIELDS
{
    CSHORT Year;
    CSHORT Month;
    CSHORT Day;
    CSHORT Hour;
    CSHORT Minute;
    CSHORT Second;
    CSHORT Milliseconds;
    CSHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;

#define TICKSPERMSEC       10000
#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define EPOCHWEEKDAY       1  /* Jan 1, 1601 was Monday */
#define DAYSPERWEEK        7
#define DAYSPERQUADRICENTENNIUM (365 * 400 + 97)
#define DAYSPERNORMALQUADRENNIUM (365 * 4 + 1)

VOID NTAPI RtlTimeToTimeFields(const LARGE_INTEGER *liTime, PTIME_FIELDS TimeFields)
{
    int SecondsInDay;
    LONG cleaps, years, yearday, months;
    LONG Days;
    LONGLONG Time;

    /* Extract millisecond from time and convert time into seconds */
    TimeFields->Milliseconds = (CSHORT) (( liTime->QuadPart % TICKSPERSEC) / TICKSPERMSEC);
    Time = liTime->QuadPart / TICKSPERSEC;

    /* The native version of RtlTimeToTimeFields does not take leap seconds
     * into account */

    /* Split the time into days and seconds within the day */
    Days = (LONG) (Time / SECSPERDAY);
    SecondsInDay = Time % SECSPERDAY;

    /* compute time of day */
    TimeFields->Hour = (CSHORT) (SecondsInDay / SECSPERHOUR);
    SecondsInDay = SecondsInDay % SECSPERHOUR;
    TimeFields->Minute = (CSHORT) (SecondsInDay / SECSPERMIN);
    TimeFields->Second = (CSHORT) (SecondsInDay % SECSPERMIN);

    /* compute day of week */
    TimeFields->Weekday = (CSHORT) ((EPOCHWEEKDAY + Days) % DAYSPERWEEK);

    /* compute year, month and day of month. */
    cleaps = (3 * ((4 * Days + 1227) / DAYSPERQUADRICENTENNIUM) + 3 ) / 4;
    Days += 28188 + cleaps;
    years = (20 * Days - 2442) / (5 * DAYSPERNORMALQUADRENNIUM);
    yearday = Days - (years * DAYSPERNORMALQUADRENNIUM)/4;
    months = (64 * yearday) / 1959;

    /* the result is based on a year starting on March.
     * To convert take 12 from Januari and Februari and
     * increase the year by one. */
    if (months < 14)
    {
        TimeFields->Month = months - 1;
        TimeFields->Year = years + 1524;
    }
    else
    {
        TimeFields->Month = months - 13;
        TimeFields->Year = years + 1525;
    }

    /* calculation of day of month is based on the wonderful
     * sequence of INT( n * 30.6): it reproduces the
     * 31-30-31-30-31-31 month lengths exactly for small n's */
    TimeFields->Day = yearday - (1959 * months) / 64 ;
    return;
}
