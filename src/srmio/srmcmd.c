/*
 * Copyright (c) 2008 Rainer Clasen
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms described in the file LICENSE included in this
 * distribution.
 *
 */

/************************************************************
 *
 *        !!!!! WARNING !!!!!
 *
 * USE THIS ONLY AT YOUR OWN RISK! 
 * THIS MIGHT DESTROY YOUR SRM POWERCONTROL.
 *
 * FYI: If any the powercontrol has only lax input checking. 
 * So by sending a garbled/misformed command you might turn
 * it into a brick. So far srmwin could fix things for me, but
 * you might be the first without this luck.
 *
 ************************************************************/

#include "srmio.h"

#include "config.h"

#include <errno.h>
#include <stdio.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#ifdef HAVE_STRING_H
# if !defined STDC_HEADERS && defined HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif



static void csvdump( srm_data_t data )
{
	srm_chunk_t *ck;

	printf(
		"time\t"
		"dur\t"
		"temp\t"
		"pwr\t"
		"speed\t"
		"cad\t"
		"hr\t"
		"\n");
	for( ck=data->chunks; *ck; ++ck ){
		printf(
			"%.1f\t"	/* "time\t" */
			"%.1f\t"	/* "dur\t" */
			"%.1f\t"	/* "temp\t" */
			"%u\t"		/* "pwr\t" */
			"%.2f\t"	/* "speed\t" */
			"%u\t"		/* "cad\t" */
			"%u\t"		/* "hr\t" */
			"\n",
			(double)(*ck)->time / 10,
			(double)data->recint / 10,
			(*ck)->temp,
			(*ck)->pwr,
			(*ck)->speed,
			(*ck)->cad,
			(*ck)->hr
			);
	}
}

static void logfunc( const char *msg )
{
	fprintf( stderr, "%s\n", msg );
}

static void usage( char *name );

int main( int argc, char **argv )
{
	char *fname = NULL;
	int opt_all = 0;
	int opt_clear = 0;
	int opt_fixup = 0;
	int opt_force = 0;
	int opt_get = 0;
	int opt_help = 0;
	int opt_int = 0;
	int opt_name = 0;
	int opt_read = 0;
	int opt_time = 0;
	int opt_verb = 0;
	char *opt_write = NULL;
	int needhelp = 0;
	struct option lopts[] = {
		{ "clear", no_argument, NULL, 'c' },
		{ "force", no_argument, NULL, 'F' },
		{ "get", optional_argument, NULL, 'g' },
		{ "help", no_argument, NULL, 'h' },
		{ "int", required_argument, NULL, 'i' },
		{ "name", no_argument, NULL, 'n' },
		{ "read", no_argument, NULL, 'r' },
		{ "time", no_argument, NULL, 't' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "write", required_argument, NULL, 'w' },
		{ "fixup", no_argument, NULL, 'x' },
	};
	char c;
	srmpc_conn_t srm;

	while( -1 != ( c = getopt_long( argc, argv, "cFg::hi:nrtvw:x", lopts, NULL ))){
		switch(c){
		  case 'c':
			++opt_clear;
			break;

		  case 'F':
			++opt_force;
			break;

		  case 'g':
			++opt_get;
		  	if( optarg && 0 == strcmp( optarg, "all" ))
				++opt_all;
			break;

		  case 'h':
		  	++opt_help;
			break;

		  case 'i':
		  	opt_int = atoi(optarg);
			break;

		  case 'n':
			++opt_name;
			break;

		  case 'r':
		  	++opt_read;
			break;

		  case 't':
			++opt_time;
			break;

		  case 'v':
			++opt_verb;
			break;

		  case 'w':
		  	opt_write = optarg;
			break;

		  case 'x':
			++opt_fixup;
			break;

		  default:
			++needhelp;
		}
	}

	if( opt_help ){
		usage( argv[0] );
		exit( 0 );
	}

	if( optind >= argc ){
		fprintf( stderr, "missing device/file name\n" );
		++needhelp;
	}
	fname = argv[optind];

	if( needhelp ){
		fprintf( stderr, "use %s --help for usage info\n", argv[0] );
		exit(1);
	}



	if( opt_read ){
		srm_data_t srmdata;

		if( NULL == (srmdata = srm_data_read( fname ))){
			fprintf( stderr, "srm_data_read(%s) failed: %s\n",
				fname, strerror(errno) );
			return 1;
		}

		if( opt_write ){
			/* TODO: check there's data */
			if( 0 > srm_data_write_srm7( srmdata, opt_write ) ){
				fprintf( stderr, "srm_data_write(%s) failed: %s\n",
					opt_write, strerror(errno) );
				return -1;
			}

		} else {
			csvdump( srmdata );
		}

		srm_data_free(srmdata);

		return 0;
	}

	if( NULL == (srm = srmpc_open( fname, opt_force, 
		opt_verb ? logfunc : NULL  ))){

		fprintf( stderr, "srmpc_open(%s) failed: %s\n",
			fname, strerror(errno) );
		return(1);
	}

	if( opt_name ){
		char *name;

		if( NULL == (name = srmpc_get_athlete( srm ))){
			perror("srmpc_get_athlete failed");
			return 1;
		}
		printf( "%s\n", name );
		free( name );
	
	} 
	
	if( opt_get ){
		srm_data_t srmdata;

		/* get new/all chunks */
		if( NULL == (srmdata = srmpc_get_data( srm, opt_all, opt_fixup ))){
			perror( "srmpc_get_data failed" );
			return(1);
		}

		if( opt_write ){
			/* TODO: check there's data */
			if( 0 > srm_data_write_srm7( srmdata, opt_write ) ){
				fprintf( stderr, "srm_data_write(%s) failed: %s\n",
					opt_write, strerror(errno) );
				return -1;
			}

		} else {
			csvdump( srmdata );
		}
		srm_data_free( srmdata );
	
	} 

	if( opt_clear ){
		if( 0 > srmpc_clear_chunks( srm ) ){
			perror( "srmpc_clear_chunks failed" );
			return 1;
		}
	}

	if( opt_int ){
		if( 0 > srmpc_set_recint( srm, opt_int ) ){
			perror( "srmpc_set_recint failed" );
			return 1;
		}
	}

	if( opt_time ){
		time_t now;
		struct tm *nows;

		time( &now );
		nows = localtime( &now );
		if( 0 > srmpc_set_time( srm, nows ) ){
			perror( "srmpc_set_time failed" );
			return 1;
		}
	}

	srmpc_close( srm );
	
	return 0;
}

static void usage( char *name )
{
	printf(
"usage: %s [options] <device_or_fname>\n"
"options: (are processed in this order)\n"
" --name              get athlete name\n"
" --get[=all]|-g      download data from SRM and dump it to stdout\n"
" --force|-F          ignore whitelist. Might be DANGEROUS\n"
" --fixup|-x          try to fix time-glitches in retrieved data\n"
" --write=<fname>|-w  save data as specified .srm file\n" 
" --read|-r           dump srm file to stdout\n"
" --clear|-c          clear data on SRM\n"
" --time|-t           set current time\n"
" --int=<interval>|-i\n"
"                     set recording interval, 10 -> 1sec\n"
"\n"
" --verbose|-v        increase verbosity\n"
" --help|-h           this cruft\n"
, name );
}

