/* Copyright (c) 1988 Regents of the University of California */

#ifndef lint
static char SCCSid[] = "$SunId$ LBL";
#endif

/*
 *  Send pictures to PC animation system.
 *
 *	6/20/88
 */

#include <stdio.h>
#include <rasterfile.h>

#include "random.h"
#include "color.h"
#include "clntrpc.h"
#include "scan.h"

#define GAMMA		2.0		/* gamma correction factor */

BYTE	gammamap[256];			/* gamma correction table */


main(argc, argv)
int	argc;
char	*argv[];
{
	char	*progname;
	int	nframes;
	int	port = PCPROGRAM;
					/* initialize */
	compgamma();
	for (progname = *argv++; --argc; argv++)
		if (!strcmp(*argv, "-p") && argv[1]) {
			port = atoi(*++argv); argc--;
		} else
			break;
	if (!argc) {
		fputs("Usage: ", stderr);
		fputs(progname, stderr);
		fputs(" [-p port] hostname [-c copies][-r record] [frame] ..\n",
				stderr);
		exit(1);
	}
	scry_open(*argv++, TCP, port);
	scry_set_compress(NONE);
	scry_set_image(TARGA_IMAGE);
	scry_set_record(PREVIEW);
	scry_set_copy_num(3);
					/* send frames */
	nframes = 0;
	for ( ; --argc; argv++)
		if (!strcmp(*argv, "-r") && argv[1]) {
			scry_set_record(atoi(*++argv)); argc--;
		} else if (!strcmp(*argv, "-c") && argv[1]) {
			scry_set_copy_num(atoi(*++argv)); argc--;
		} else {
			sendframe(*argv);
			nframes++;
		}
	if (nframes == 0)		/* get stdin */
		sendframe(NULL);
					/* clean up */
	scry_close();
	exit(0);
}


sendframe(file)			/* convert and send a frame */
char	*file;
{
	COLR	scanin[SCANLINE];
	int	xres, yres;
	int	xbeg, ybeg;
	FILE	*fp;
	int	j;
	register int	k, c;
	register BYTE	*in;
	short	*out;
						/* open file */
	if (file == NULL) {
		fp = stdin;
		file = "<stdin>";
	} else if ((fp = fopen(file, "r")) == NULL) {
		perror(file);
		exit(1);
	}
						/* get dimensions */
	getheader(fp, NULL);
	if (fgetresolu(&xres, &yres, fp) != (YMAJOR|YDECR) ||
			xres > SCANLINE || yres > NUMSCANS) {
		fputs(file, stderr);
		fputs(": bad picture size\n", stderr);
		exit(1);
	}
						/* compute center */
	xbeg = (SCANLINE-xres)/2;
	ybeg = (NUMSCANS-yres)/2;
						/* clear output */
	bzero(sc_frame_arr, sizeof(sc_frame_arr));
						/* get frame */
	for (j = yres-1; j >= 0; j--) {
		if (freadcolrs(scanin, xres, fp) < 0) {
			fputs(file, stderr);
			fputs(": read error\n", stderr);
			exit(1);
		}
		for (in = scanin[0], out = sc_frame_arr[ybeg+j]+xbeg;
				in < scanin[xres]; in += 4, out++) {
			k = in[EXP] - COLXS;
			if (k >= 8) {
				in[RED] =
				in[GRN] =
				in[BLU] = 255;
			} else if (k <= -8) {
				in[RED] =
				in[GRN] =
				in[BLU] = 0;
			} else if (k > 0) {
				c = in[RED] << k;
				in[RED] = c > 255 ? 255 : c;
				c = in[GRN] << k;
				in[GRN] = c > 255 ? 255 : c;
				c = in[BLU] << k;
				in[BLU] = c > 255 ? 255 : c;
			} else if (k < 0) {
				in[RED] = in[RED] >> -k;
				in[GRN] = in[GRN] >> -k;
				in[BLU] = in[BLU] >> -k;
			}
			for (k = 0; k < 3; k++)
				*out = *out<<5 | (gammamap[in[k]]+(random()&0x7))>>3;
		}
	}
						/* send frame */
	scry_send_frame();
						/* close file */
	fclose(fp);
}


compgamma()				/* compute gamma correction map */
{
	extern double  pow();
	register int  i, val;

	for (i = 0; i < 256; i++) {
		val = pow(i/256.0, 1.0/GAMMA) * 256.0;
		if (val > 248) val = 248;
		gammamap[i] = val;
	}
}
