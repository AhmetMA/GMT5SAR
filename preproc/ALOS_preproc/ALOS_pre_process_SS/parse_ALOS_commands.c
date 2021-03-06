#include "image_sio.h"
#include "lib_functions.h"

/* reads options 				*/
/* start with third arguement			*/
void parse_ALOS_commands(int na, char **a, char *USAGE, struct PRM *prm, int *nsub, int *burst_skip, int *num_burst)
{
int	n;

for(n = 3; n < na; n++) {
	if (!strcmp(a[n], "-near")) {
		n++;
		if (n == na) die (" no option after -near!\n",""); 
		prm->near_range = atof(a[n]);
		fprintf(stderr," setting near_range to %9.2lf \n",prm->near_range);
	} else if (!strcmp(a[n], "-swath")) {
		n++;
		if (n == na) die (" no option after -swath\n",""); 
		*nsub = atoi(a[n]) - 1;
		fprintf(stderr," setting swath to %d \n",*nsub+1);
	} else if (!strcmp(a[n], "-burst_skip")) {
		n++;
		if (n == na) die (" no option after -burst_skip\n",""); 
		*burst_skip = atoi(a[n]);
		fprintf(stderr," setting burst_skip to %d \n",*burst_skip);
	} else if (!strcmp(a[n], "-num_burst")) {
		n++;
		if (n == na) die (" no option after -num_burst\n",""); 
		*num_burst = atoi(a[n]);
		fprintf(stderr," setting num_nburst to %d \n",*num_burst);
	} else if (!strcmp(a[n], "-radius")) {
		n++;
		if (n == na) die (" no option after -radius!\n",""); 
		prm->RE = atof(a[n]);
		fprintf(stderr," setting radius to %f \n",prm->RE);
	} else if (!strcmp(a[n], "-force_slope")) {
		n++;
		if (n == na) die (" no option after -force_slope!\n","");
		force_slope = 1;
		forced_slope = atof(a[n]);
		fprintf(stderr," setting chirp slope to %f \n",forced_slope);
	} else if (!strcmp(a[n], "-fd1")) {
		n++;
		if (n == na) die (" no option after -fd1!\n","");
		prm->fd1 = atof(a[n]);
		dopp = 0;
		fprintf(stderr," setting fd1 to %f \n",prm->fd1);
	} else if (!strcmp(a[n], "-chirp_ext")) {
		n++;
		if (n == na) die (" no option after -chirp_ext!\n","");
		prm->chirp_ext = atoi(a[n]);
		fprintf(stderr," setting chirp extent to %d \n",prm->chirp_ext);
	} else if (!strcmp(a[n], "-nrows")) {
		n++;
		if (n == na) die (" no option after -nrows!\n",""); 
		prm->nrows = atoi(a[n]);
		fprintf(stderr," setting nrows to %d \n",prm->nrows);
	} else if (!strcmp(a[n], "-nodopp")) {
		dopp = 0;
		prm->fd1 = prm->fdd1 = prm->fddd1 = 0.0;
		fprintf(stderr," no doppler calculation (sets to zero!) \n");
	} else if (!strcmp(a[n], "-quad")) {
		quad_pol = 1;
		fprintf(stderr," quad pol data: divide PRF by 2; defining chirp_slope) \n");
	} else if (!strncmp(a[n], "-debug",2)) {
		verbose = debug = 1;
		fprintf(stderr," debug and verbose output \n");
	} else if (!strncmp(a[n], "-V",1)) {
		verbose = 1;
		fprintf(stderr," verbose output \n");
	} else if (!strncmp(a[n], "-v",1)) {
		verbose = 1;
		fprintf(stderr," verbose output \n");
	} else {
		fprintf(stderr," %s *** option not recognized ***\n\n",a[n]);
		fprintf(stderr,"%s",USAGE);
		exit(1);
		}
	}
}
