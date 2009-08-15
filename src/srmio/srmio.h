/*
 * Copyright (c) 2008 Rainer Clasen
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms described in the file LICENSE included in this
 * distribution.
 *
 */

#ifndef _SRMIO_H
#define _SRMIO_H

#include "srmio_config.h"

# ifdef __cplusplus
extern "C"
{
# endif

/************************************************************
 *
 * from srmdata.c
 *
 ************************************************************/

typedef uint64_t srm_time_t;	/* seconds since epoch * 10 */

struct _srm_chunk_t {
	srm_time_t	time;	/* chunk end time */
	double		temp;	/* temperature °C */
	unsigned	pwr;	/* avg power W */
	double		speed;	/* avg speed km/h */
	unsigned	cad;	/* avg cadence 1/min */
	unsigned	hr;	/* avg hr 1/min */
	int		ele;	/* elevation in meter */
};
typedef struct _srm_chunk_t *srm_chunk_t;

srm_chunk_t srm_chunk_new( void );
srm_chunk_t srm_chunk_clone( srm_chunk_t chunk );
void srm_chunk_free( srm_chunk_t chunk );



struct _srm_marker_t {
	unsigned	first;
	unsigned	last;
	char		*notes;
};
typedef struct _srm_marker_t *srm_marker_t;

srm_marker_t srm_marker_new( void );
void srm_marker_free( srm_marker_t marker );



struct _srm_data_t {
	srm_time_t	recint;
	double		slope;
	int		zeropos;
	int		circum;
	char		*notes;

	srm_chunk_t	 *chunks;
	unsigned	cused;
	unsigned	cavail;

	srm_marker_t	 *marker;
	unsigned	mused;
	unsigned	mavail;
};
typedef struct _srm_data_t *srm_data_t;

/* alloc and initialize empty data */
srm_data_t srm_data_new( void );

int srm_data_add_chunkp( srm_data_t data, srm_chunk_t chunk );
int srm_data_add_chunk( srm_data_t data, srm_chunk_t chunk );
int srm_data_add_markerp( srm_data_t data, srm_marker_t mark );
int srm_data_add_marker( srm_data_t data, unsigned first, unsigned last );
srm_marker_t *srm_data_blocks( srm_data_t data );

/* free the list: */
void srm_data_free( srm_data_t data );



/************************************************************
 *
 * from srmfile.c
 *
 ************************************************************/

srm_data_t srm_data_read( const char *fname );
int srm_data_write_srm7( srm_data_t data, const char *fname );




/************************************************************
 *
 * from srmio.c
 *
 ************************************************************/

typedef void (*srmpc_log_callback_t)( const char *msg );
struct _srmpc_conn_t {
	int		fd;
	struct termios	oldios;
	int		stxetx;
	srmpc_log_callback_t	lfunc;
};
typedef struct _srmpc_conn_t *srmpc_conn_t;


srmpc_conn_t srmpc_open( const char *fname, int force,
	srmpc_log_callback_t lfunc );
void srmpc_close( srmpc_conn_t conn );



char *srmpc_get_athlete( srmpc_conn_t conn );

int srmpc_get_time( srmpc_conn_t conn, struct tm *timep ); 
int srmpc_set_time( srmpc_conn_t conn, struct tm *timep ); 

int srmpc_get_circum( srmpc_conn_t conn );

double srmpc_get_slope( srmpc_conn_t conn );

int srmpc_get_zeropos( srmpc_conn_t conn );

int srmpc_get_recint( srmpc_conn_t conn );
int srmpc_set_recint( srmpc_conn_t conn, srm_time_t recint );



struct _srmpc_get_chunk_t {
	/* whole downlad */
	srmpc_conn_t		conn;
	int			fixup;
	struct tm		pctime;
	unsigned		blocks;

	/* current block */
	unsigned		blocknum;
	srm_time_t		bstart;
	unsigned		dist;
	int			temp;	
	srm_time_t		recint;

	/* current chunk */
	unsigned		chunknum;	/* within block */
	int			isfirst;	/* ... of marker */
	int			iscont;		/* ... of marker */
	struct _srm_chunk_t	chunk; /* TODO: hack? should use srm_chunk_new()? */

	void			*cbdata;
};
typedef struct _srmpc_get_chunk_t *srmpc_get_chunk_t;

typedef int (*srmpc_chunk_callback_t)( srmpc_get_chunk_t gh );
int srmpc_get_chunks( srmpc_conn_t conn, int getall, int fixup,
	srmpc_chunk_callback_t cfunc, void *cbdata );

int srmpc_clear_chunks( srmpc_conn_t conn );

srm_data_t srmpc_get_data( srmpc_conn_t conn, int getall, int fixup );


# ifdef __cplusplus
}
# endif


#endif /* _SRMIO_H */
