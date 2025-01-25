%require "3.8.2"
%language "C++"
%header "tjsdate.tab.hpp"
%output "tjsdate.tab.cpp"
%define api.namespace {TJSDate}

%code top {
#include <malloc.h>

#include "tjsTypes.h"
#include "tjsDateParser.h"
}

%code requires {
#include "tjsTypes.h"

namespace TJSDate {
    class tTJSDateParser;
}
}

%parse-param { tTJSDateParser *ptr }
%lex-param   { tTJSDateParser *ptr }

%union {
	tjs_int32 val;
}


%token				DP_AM DP_PM
%token <val>		DP_NUMBER
%token <val>		DP_MONTH
%token <val>		DP_WDAY
%token <val>		DP_TZ


%%

/*---------------------------------------------------------------------------*/
/* input                                                                     */
/*---------------------------------------------------------------------------*/



input
	: date_time_string
;


/*---------------------------------------------------------------------------*/
/* rules                                                                     */
/*---------------------------------------------------------------------------*/

date_time_string
	/* Sun, 3 May 2004 11:22:33 GMT +900 (JST) */
	:	wday_omittable
		DP_NUMBER
		month DP_NUMBER
		time
		tz_omittable									{	ptr->SetMDay($2);
															ptr->SetYear($4); }
	/* Sun, 3-May 2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		DP_NUMBER '-'
		month DP_NUMBER
		time
		tz_omittable									{	ptr->SetMDay($2);
															ptr->SetYear($5); }
	/* Sun, 3-May-2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		DP_NUMBER '-'
		month '-' DP_NUMBER
		time
		tz_omittable									{	ptr->SetMDay($2);
															ptr->SetYear($6); }
	/* Sun, May 3 2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		month
		DP_NUMBER DP_NUMBER
		time
		tz_omittable									{	ptr->SetMDay($3);
															ptr->SetYear($4); }
	/* Sun, May-3 2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		month '-'
		DP_NUMBER DP_NUMBER
		time
		tz_omittable									{	ptr->SetMDay($4);
															ptr->SetYear($5); }
	/* Sun, May-3-2004 11:22:33 GMT +900 (JST) */
	|	wday_omittable
		month '-'
		DP_NUMBER '-' DP_NUMBER
		time
		tz_omittable									{	ptr->SetMDay($4);
															ptr->SetYear($6); }
	/* Sun, 3 May 11:22:33 2004 GMT +900 (JST) */
	|	wday_omittable
		DP_NUMBER
		month
		time
		DP_NUMBER
		tz_omittable									{	ptr->SetMDay($2);
															ptr->SetYear($5); }
	/* Sun, 3-May 11:22:33 2004 GMT +900 (JST) */
	|	wday_omittable
		DP_NUMBER '-'
		month
		time
		DP_NUMBER
		tz_omittable									{	ptr->SetMDay($2);
															ptr->SetYear($6); }
	/* Sun, May 3 11:22:33 2004 GMT +900 (JST) */
	|	wday_omittable
		month DP_NUMBER
		time
		DP_NUMBER
		tz_omittable									{	ptr->SetMDay($3);
															ptr->SetYear($5); }
	/* Sun, May-3 11:22:33 2004 GMT +900 (JST) */
	|	wday_omittable
		month '-' DP_NUMBER
		time
		DP_NUMBER
		tz_omittable									{	ptr->SetMDay($4);
															ptr->SetYear($6); }
	/* 2004/03/03 11:22:33 */
	|	wday_omittable
		DP_NUMBER hyphen_or_slash
		DP_NUMBER hyphen_or_slash
		DP_NUMBER
		time
		tz_omittable									{	ptr->SetMonth($4-1);
															ptr->SetYear($2);
															ptr->SetMDay($6); }
;

wday
	:	DP_WDAY
;

wday_omittable
	:	wday ','
	|	wday
	|	/* empty */
;


month
	:	DP_MONTH										{ ptr->SetMonth($1); }
;

hyphen_or_slash
	:	'-'
	|	'/'
;


time_sub_sec_omittable
	:	'.' DP_NUMBER									{ /* TODO: sub-seconds support */ }
	|	/* empty */
;

time_hms
	:	DP_NUMBER ':'
		DP_NUMBER ':'
		DP_NUMBER time_sub_sec_omittable
		{
			ptr->SetHours($1);
			ptr->SetMin($3);
			ptr->SetSec($5);
		}
	|	DP_NUMBER ':'
		DP_NUMBER
		{
			ptr->SetHours($1);
			ptr->SetMin($3);
			ptr->SetSec(0);
		}
;

am_or_pm
	:	DP_AM											{ ptr->SetAMPM(false); }
	|	DP_PM											{ ptr->SetAMPM(true); }
;

time
	:	time_hms
	|	am_or_pm time_hms
	|	time_hms am_or_pm
;

tz_name_omittable
	:	DP_TZ											{ ptr->SetTimeZone($1); }
	|	/* empty */
;

tz_offset_omittable
	:	'+' DP_NUMBER									{ ptr->SetTimeZoneOffset($2); }
	|	'-' DP_NUMBER									{ ptr->SetTimeZoneOffset(-$2); }
	|	/* empty */
;



tz_desc_omittable
	:	'('												{ ptr->SkipToRightParenthesis(); }
		')'
	|	/* empty */
;

tz_omittable
	:	tz_name_omittable
		tz_offset_omittable
		tz_desc_omittable
;

/*---------------------------------------------------------------------------*/
%%