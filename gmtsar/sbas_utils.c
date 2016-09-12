/*      $Id: sbas.h 39 2016-06-18 03/16/24 Xiaohua Xu $  */
/*****************************************************************************************
 *  Program to do InSAR time-series analysis.                                            *
 *  Use Small Baseline Subset (SBAS) algorithm.                                          *
 *                                                                                       *
 *  Xiaohua Xu and David T. Sandwell, Jul, 2016                                          *
 *                                                                                       *
 *  Taking the old sbas code to add atmospheric correction by means of common point      *
 *  stacking by Tymofeyeva & Fialko 2015.                                                *
 *                                                                                       *
 ***************************************************************************************** 
 * Creator: Xiaopeng Tong and David Sandwell 						 *
 *          (Scripps Institution of Oceanography)					 *
 * Date: 12/23/2012  									 *
 ****************************************************************************************/ 
/*****************************************************************************************
 *  Modification history: 								 *
 *  08/31/2013 debug the program							 *
 *  03/20/2014 add DEM error, mean velocity 						 *
 *  03/22/2014 add correlation, use weighted least-squares  				 *
 *  04/01/2014 add seasonal term			  				 *
 *  08/19/2014 allocate memory with 1D array instead of multiple malloc                  *
 *  08/19/2014 do not require the velocity curve go through origin                       *
 *  08/19/2014 remove seasonal term                                                      *
 *  08/19/2014 fix temporal smoothing                                                    *
 ****************************************************************************************/

/* Reference: 
P. Berardino, G. Fornaro, R. Lanari, and E. Sansosti, “A new algorithm
for surface deformation monitoring based on small baseline differential
SAR interferograms,” IEEE Trans. Geosci. Remote Sensing, vol. 40, pp.
2375–2383, Nov. 2002.

Schmidt, D. A., and R. Bürgmann(2003),
Time-dependent land uplift and subsidence in the Santa Clara valley, 
California, from a large interferometric synthetic aperture radar data set, 
J. Geophys. Res., 108, 2416, doi:10.1029/2002JB002267, B9.
*/

/* Use DGELSY to solve the equations */
/* Calling DGELSY using column-major order */

#include "gmtsar.h"
# define Malloc(type,n) (type *)malloc((n)*sizeof(type))
#ifndef max
# 	define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif
# ifdef DEBUG
# define checkpoint() printf("Checkpoint at line %d in file %s\n", __LINE__, __FILE__) 
# else 
# define checkpoint()
# endif

char *sbas_USAGE = " \n\nUSAGE: sbas intf.tab scene.tab N S xdim ydim [-smooth sf] [-wavelength wl] [-incidence inc] [-range -rng] [-rms] [-dem]\n\n"
" input: \n"
"intf.tab               --  list of unwrapped (filtered) interferograms:\n"
"   format:   unwrap.grd  corr.grd  ref_id  rep_id  B_perp \n"
"scene.tab              --  list of the SAR scenes in chronological order\n"
"   format:   scene_id   number_of_days \n"
"   note:     the number_of_days is relative to a reference date \n"
"N                      --  number of the interferograms\n"
"S                      --  number of the SAR scenes \n"
"xdim and ydim          --  dimension of the interferograms\n"
"-smooth sf             --  smoothing factors, default=0 \n"
"-wavelength wl         --  wavelength of the radar wave (m) default=0.236 \n"
"-incidence theta       --  incidence angle of the radar wave (degree) default=37 \n" 
"-range rng             --  range distance from the radar to the center of the interferogram (m) default=866000 \n" 
"-rms                   --  output RMS of the data misfit grids (mm): rms.grd\n" 
"-dem                   --  output DEM error (m): dem.grd \n\n" 
" output: \n"
"disp_##.grd            --  cumulative displacement time series (mm) grids\n"
"vel.grd                --  mean velocity (mm/yr) grids \n\n"
" example:\n"
"   sbas intf.tab scene.tab 88 28 700 1000 \n\n";

void dgelsy_(const int *m, const int *n,const int *nrhs,double *G, const int *lda,double* b, const int *ldb,int *jpvt,const double *rcond, const int *rank,double *work,const int *lwork,const int *info);
void die (char *, char *);
int write_output_ts(void *API, struct GMT_GRID *Out,int agc,char **agv, int xdim, int ydim, int S, int flag_rms, int flag_dem, float *disp, float *vel, float *res, float *dem, float *screen, double wl);
int free_memory_ts(int N,float *phi,float *var,char **gfile,char **cfile,float *disp,double *G,double *A,double *Gs,int *H,double *d,double *ds,int *L,float *res,float *vel,double *time,int *flag,float *bperp,float *dem,double *work,int *jpvt,int *hit);
int allocate_memory_ts(int **jpvt,double **work,double **d,double **ds,float **bperp,char ***gfile,char ***cfile,int **L,double **time,int **H,double **G,double **A,double **Gs,int **flag,float **dem,float **res,float **vel,float **phi,float **var,float **disp,int n,int m,int lwork,int ldb,int N,int S,int xdim,int ydim, int **hit);
//int remove_ts(float *phi, float *ts, int xdim, int ydim, int N, int S, int *H, int *L);


int parse_command_ts(int agc, char **agv, float *sf ,double *wl, double *theta, double *rng, int *flag_rms, int *flag_dem) {

	int i;

	for (i=7;i<agc;i++) {
		if (!strcmp(agv[i],"-smooth")) { 
			i++;
			if (i==agc) die("no option after -smooth!\n","");
			*sf=atof(agv[i]);
		} else if (!strcmp(agv[i],"-wavelength")) {
			i++;
			if (i==agc) die("no option after -wavelength!\n","");
			*wl=atof(agv[i]);
		} else if (!strcmp(agv[i],"-incidence")) {
			i++;
			if (i==agc) die("no option after -incidence! \n","");
			*theta=atof(agv[i]);
		} else if (!strcmp(agv[i],"-range")) {
			i++;
			if (i==agc) die("no option after -range \n","");
			*rng=atof(agv[i]);
		} else if (!strcmp(agv[i],"-rms")) {
			*flag_rms=1;
			fprintf(stderr,"compute RMS misfit\n");
		} else if (!strcmp(agv[i],"-dem")) {
			*flag_dem=1;
			fprintf(stderr,"compute DEM error\n");
		} else {
		fprintf(stderr," %s *** option not recognized ***\n\n",agv[i]);
		fprintf(stderr,"%s",sbas_USAGE);
		exit(1);
		}
	}

	fprintf(stderr,"\nsetting smoothing to %7.3lf \n", *sf);
	fprintf(stderr,"setting radar wavelength to %7.3lf m \n",*wl);
	fprintf(stderr,"setting radar incidence angle to %7.3lf degree \n", *theta);
	fprintf(stderr,"setting range to %9.3lf m \n", *rng);

	return(1);

}


int allocate_memory_ts(int **jpvt,double **work,double **d,double **ds,float **bperp,char ***gfile,char ***cfile,int **L,double **time,int **H,double **G,double **A,double **Gs,int **flag,float **dem,float **res,float **vel,float **phi,float **var,float **disp,int n,int m,int lwork,int ldb,int N,int S,int xdim,int ydim, int **hit) {

	int i;
	char **p1,**p2;

	if ((*jpvt = Malloc(int,n)) == NULL) die("memory allocation!","jpvt");
	if ((*work = Malloc(double, lwork)) == NULL) die("memory allocation!","work");
	if ((*d = Malloc(double,ldb)) == NULL) die("memory allocation!","d");
	if ((*ds = Malloc(double,N)) == NULL) die("memory allocation!","ds");
	if ((*bperp = Malloc(float,N)) == NULL) die("memory allocation!","bperp");
	if ((*gfile = Malloc(char *,N)) == NULL) die("memory allocation!","gfile");
	if ((*cfile = Malloc(char *,N)) == NULL) die("memory allocation!","cfile");
	p1 = *gfile; p2 = *cfile;
	for (i=0;i<N;i++) {
		if ((p1[i] = Malloc(char,36)) == NULL) die("memory allocation!","gfile[i]");
		if ((p2[i] = Malloc(char,36)) == NULL) die("memory allocation!","cfile[i]");
	}
	if ((*L = Malloc(int,S)) == NULL) die("memory allocation!","L");
	if ((*time = Malloc(double, S)) == NULL) die("memory allocation!","time");
	if ((*H = Malloc(int,N*2)) == NULL) die("memory allocation!","H");
	if ((*G = Malloc(double, m*n)) == NULL) die("memory allocation!","G");
	if ((*A = Malloc(double, m*n)) == NULL) die("memory allocation!","A");
	if ((*Gs = Malloc(double, N*n)) == NULL) die("memory allocation!","Gs");
	if ((*flag = Malloc(int, xdim * ydim)) == NULL) die("memory allocation!","flag");
	if ((*dem = Malloc(float, xdim * ydim)) == NULL) die("memory allocation!","dem");
	if ((*res = Malloc(float, xdim * ydim)) == NULL) die("memory allocation!","res");
	if ((*vel = Malloc(float, xdim * ydim)) == NULL) die("memory allocation!","vel");
	if ((*phi = Malloc(float, N*xdim*ydim)) == NULL) die("memory allocation!","phi");
	if ((*var = Malloc(float, N*xdim*ydim)) == NULL) die("memory allocation!","var");
	if ((*disp = Malloc(float, S*xdim*ydim)) == NULL) die("memory allocation!","disp");

	if ((*hit = Malloc(int, S*S)) == NULL) die("memory allocation!","hit");
	printf("Memory Allocation Successful...\n");
	return(1);
}


int init_array_ts(double *G, double *Gs, float *res, float *dem, float *disp, int n, int m, int xdim, int ydim, int N, int S) {

	int i,j,k,p;

	for (i=0;i<m*n;i++) G[i]=0;
	for (i=0;i<n*N;i++) Gs[i]=0;

	for (k=0;k<ydim;k++) {
		for (j=0;j<xdim;j++) {
			res[j+xdim*k]=0;
			dem[j+xdim*k]=0;
			for (p=0;p<S;p++) { 
				disp[p*xdim*ydim+j*ydim+k]=0;
			}
		}
	}
	return(1);
}


int read_table_data_ts(void *API,FILE *infile,FILE *datefile,char **gfile,char **cfile,int *H,float *bperp,int *flag,float *var,float *phi,int S,int N,int xdim,int ydim,struct GMT_GRID **Out, int *L, double *time) {

	char tmp1[200],tmp2[200],tmp3[200];
	int i,j,k,xin,yin;
	float *corin, *grdin;
	struct  GMT_GRID *CC = NULL, *GG = NULL;

	printf("read table file ...\n");
	/* read in scene.tab */
	i=0;
	while(fscanf(datefile, "%s %s", &tmp1[0],&tmp2[0]) == 2) {
		if (feof(datefile)) break;
		L[i]=atoi(tmp1);
		time[i]=atof(tmp2);
		i=i+1;
	}
	if (i != S) die("S and number of the SAR scenes don't match!","");
	fprintf(stderr,"number of SAR scenes is %d \n", S);
	for (i=S-1;i>=0;i--) time[i]=time[i]-time[0];

	/* read in intf.tab */
	i=0;
	while(fscanf(infile, "%s %s %s %s %s", gfile[i],cfile[i],&tmp1[0],&tmp2[0],&tmp3[0]) == 5) {
		if (feof(infile)) break;
		H[i*2+0]=atoi(tmp1);
		H[i*2+1]=atoi(tmp2);
		bperp[i]=atof(tmp3);
		i=i+1;
	}
	if (i != N) die("N and number of interferograms don't match!","");
	fprintf(stderr,"number of interferograms is %d \n", N);


	/* read in N 2-dimensional grd file into 3D array */
	printf("read phase and correlation grids ...\n");
	for (i=0;i<N;i++) {
		if ((CC = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER_ONLY, NULL, cfile[i], NULL)) == NULL) die("Can't open ",cfile[i]);
		xin = CC->header->nx;	yin = CC->header->ny;
		if (xin != xdim || yin != ydim) die("dimension don't match!",cfile[i]);
		if ((GG = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_HEADER_ONLY, NULL, gfile[i], NULL)) == NULL) die("Can't open ",gfile[i]);
		xin = GG->header->nx;	yin = GG->header->ny;
		if (xin != xdim || yin != ydim) die("dimension don't match!",gfile[i]);
 		if ((CC = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA_ONLY, NULL, cfile[i], CC)) == NULL) die("Can't read ",cfile[i]);
		if ((GG = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_DATA_ONLY, NULL, gfile[i], GG)) == NULL) die("Can't read ",gfile[i]);
		corin = CC->data;	grdin = GG->data;
		for (k=0;k<ydim;k++) {
			for (j=0;j<xdim;j++) {
				phi[i*xdim*ydim+ydim*j+k] = grdin[j+k*xdim];
				if (isnan(grdin[j+k*xdim]) != 0) { 
					flag[j+xdim] = 1;
				}
				if (corin[j+k*xdim]>=1e-2 && corin[j+k*xdim]<=0.99) {
				/* Rosen et al., 2000 IEEE */
					var[i*xdim*ydim+ydim*j+k] = sqrt((1.0-corin[j+k*xdim]*corin[j+k*xdim])/(corin[j+k*xdim]*corin[j+k*xdim])); 
				}
				else if (corin[j+k*xdim]<1e-2) {
					var[i*xdim*ydim+j*ydim+k] = 99.99;
				}
				else {
					var[i*xdim*ydim+j*ydim+k] = 0.1;
				}
			}
		}
		if (*Out == NULL && (*Out = GMT_Duplicate_Data (API, GMT_IS_GRID, GMT_DUPLICATE_DATA, CC)) == NULL) die("error creating output grid","");	/* Do this once */
		if (GMT_Destroy_Data (API, &CC)) die("error freeing data",cfile[i]);
		if (GMT_Destroy_Data (API, &GG)) die("error freeing data",gfile[i]);
	}
	return(1);
}

int init_G_ts(double *G, double *Gs, int N, int S, int m, int n, int *L, int *H, double *time, float sf, float *bperp, double scale) {

	int i,j;

	printf("fill the G matrix ...\n");
	for (i=0;i<N;i++) {
		for (j=0;j<S-1;j++) {
			if (L[j]>=H[i*2+0] && L[j]<H[i*2+1]) {
				G[i+m*j]=1;
				Gs[i+N*j]=1;
			}
		}
		G[i+m*(n-1)]=bperp[i]*scale;
		Gs[i+N*(n-1)]=bperp[i]*scale;
	}

	for (i=0;i<S-2;i++) {
		G[i+N+i*m]=sf/(time[i+1]-time[i]);
		G[i+N+(i+1)*m]=-sf/(time[i+2]-time[i+1]);
	}
	return(1);
}


int lsqlin_sov_ts(int xdim, int ydim, float *disp, float *vel, int *flag, double *d, double *ds,double *time, double *G, double *Gs, double *A, float *var, float *phi, int N, int S,int m, int n, double *work,int lwork, int flag_dem, float *dem, int flag_rms, float *res, int *jpvt, double wl) {

	int i,j,k,p,info=0;
	int rank=0,nrhs=1,lda,ldb;
	double rcond = 1e-3,pred;
	double sumxx,sumxy,sumx,sumy;
	float new,old;

	lda=max(1,m);
	ldb=max(1,max(m,n));

	printf("run least-squares problem over %d by %d pixel ...\n",xdim,ydim);
	for (k=0;k<ydim;k++) {
		for (j=0;j<xdim;j++) {
			/* check the dummy value of the grd file */
			if (flag[j+xdim*k] != 1) {
				for (i=0;i<m;i++) { 
					d[i]=0;
			       		if (i<N) { 
						d[i] = (double)phi[i*xdim*ydim+ydim*j+k]/var[i*xdim*ydim+ydim*j+k];
						ds[i] = d[i];
					}
				}

				for (i=0;i<m;i++) { 
					for (p=0;p<n;p++) { 
						if (i<N) {
							G[i+m*p]=A[i+m*p]/var[i*xdim*ydim+ydim*j+k]; 
						}
						else {
							G[i+m*p]=A[i+m*p]; 
						}
					}
				}
  
				dgelsy_(&m,&n,&nrhs,G,&lda,d,&ldb,jpvt,&rcond,&rank,work,&lwork,&info);

				if (info != 0) printf("warning! input has an illegal value\n");
				if (j==0 && k==0) { 
					if (rank==n) { 
						printf("matrix is full rank: %d\n\n", rank);
					}
					else if (rank<n) { 
						printf("matrix is rank-deficient: %d\n\n", rank);
					}
				}
				for (i=0;i<S;i++) { 
					for (p=0;p<i;p++) { 
						disp[i*xdim*ydim+j*ydim+k]=disp[i*xdim*ydim+j*ydim+k]+d[p];
					}
					//disp[i*xdim*ydim+j*ydim+k]=-79.58*wl*disp[i*xdim*ydim+j*ydim+k];  //1000/4/pi
				}

				if (flag_dem == 1) dem[j+xdim*k]=d[n-1];

				if (flag_rms == 1) {
					new=0;
					old=0;
				/* check the WRMS reduction */
					for (i=0;i<N;i++) {
						pred=0;
						for (p=0;p<n;p++) {
							pred=pred+d[p]*Gs[i+p*N]/var[i*xdim*ydim+ydim*j+k];
						}
						new=new+(ds[i]-pred)*(ds[i]-pred);
						old=old+ds[i]*ds[i];
					}	
					res[j+xdim*k]=(sqrt(old)-sqrt(new))/sqrt(old);
				}

				/* fitting a straight line by least squares */
				sumxy=0;
				sumxx=0;
				sumx=0;
				sumy=0;
				for (i=0;i<S;i++) {
					sumxy=sumxy+time[i]*disp[i*xdim*ydim+j*ydim+k];
					sumxx=sumxx+time[i]*time[i];
					sumy=sumy+disp[i*xdim*ydim+j*ydim+k];
					sumx=sumx+time[i];
				}
				vel[j+xdim*k]=-79.58*wl*(S*sumxy-sumx*sumy)/(S*sumxx-sumx*sumx)*365.0;
			}
			else { 
				for (i=0;i<S;i++) disp[i*xdim*ydim+j*ydim+k] = NAN;
				vel[j+xdim*k] = NAN;
				if (flag_rms == 1) res[j+xdim*k] = NAN;
				if (flag_dem == 1) dem[j+xdim*k] = NAN;
			}
		}
	}

	return(1);
}


int write_output_ts(void *API, struct GMT_GRID *Out,int agc,char **agv, int xdim, int ydim, int S, int flag_rms, int flag_dem, float *disp, float *vel, float *res, float *dem, float *screen, double wl){

	int i,j,k;
	float *grdin, *save_grid;
	char tmp1[200],outfile[200];

	printf("write output ...\n");

	strcpy(Out->header->title,"");
	strcpy(Out->header->remark,"");
       
	strcpy(tmp1,"");
	for (i=0;i<agc;i++) {
		strcat(tmp1,agv[i]); 
		strcat(tmp1," ");
	}
	if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_COMMAND, tmp1, Out)) die("could not set title","");
	
	//strcpy(Out->header->title,"displacement time series (mm)");
	grdin = Out->data;
	for (i=0;i<S;i++) {
		strcpy(outfile,"disp_");
		for (k=0;k<ydim;k++) {
			for (j=0;j<xdim;j++) {
				//disp[i*xdim*ydim+j*ydim+k]=-79.58*wl*disp[i*xdim*ydim+j*ydim+k];  //1000/4/pi
				grdin[j+k*xdim]=-79.58*wl*disp[i*xdim*ydim+j*ydim+k]; 
		      }
		}
		sprintf(tmp1,"%03d",i+1);
		strcat(outfile,tmp1);
		strcat(outfile,".grd");
		sprintf(tmp1,"Displacement Time Series %03d",i+1);
		strcpy(Out->header->title,"");
		strcpy(Out->header->remark,"");
		if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_REMARK, "Displacement Time Series (mm)", Out)) die("could not set title","");
		if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_TITLE, tmp1, Out)) die("could not set title","");
 		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, outfile, Out)) {
			die("Failed to write output grid",outfile);
		}
	}
	save_grid = Out->data;	/* Remember the original grid */
	if (flag_rms == 1) {
		Out->data = res;
		sprintf(outfile,"rms.grd");
		strcpy(Out->header->title,"");
		strcpy(Out->header->remark,"");
		if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_REMARK, "WRMS reduction from SBAS (mm)", Out)) die("could not set title","");
		if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_TITLE, "Weighed Root Mean Square of Fitting", Out)) die("could not set title","");
 		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, outfile, Out)) {
			die("Failed to write output grid",outfile);
		}
	}

	if (flag_dem == 1) {
		Out->data = dem;
		sprintf(outfile,"dem.grd");
		strcpy(Out->header->title,"");
		strcpy(Out->header->remark,"");
		if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_REMARK, "DEM error estimated from SBAS (m)", Out)) die("could not set title","");
		if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_TITLE, "Digital Elevation Model Error", Out)) die("could not set title","");
 		if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, outfile, Out)) {
			die("Failed to write output grid",outfile);
		}
	}
	if (1 == 1) {
		grdin = Out->data;
		for (i=0;i<S;i++){
			strcpy(outfile,"aps_");
			for (k=0;k<ydim;k++) {
				for (j=0;j<xdim;j++) {
					grdin[j+k*xdim]=screen[i*xdim*ydim+j*ydim+k]; 
				}
			}
			sprintf(tmp1,"%03d",i+1);
			strcat(outfile,tmp1);
			strcat(outfile,".grd");
			sprintf(tmp1,"Atmospheric Phase Screen %03d",i+1);
			strcpy(Out->header->title,"");
			strcpy(Out->header->remark,"");
			if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_REMARK, "Atmospheric Phase Screen", Out)) die("could not set title","");
			if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_TITLE, tmp1, Out)) die("could not set title","");
			if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, outfile, Out)) {
				die("Failed to write output grid",outfile);
			}
		}
	}

	Out->data = vel;
	sprintf(outfile,"vel.grd");
	strcpy(Out->header->title,"");
	strcpy(Out->header->remark,"");
	if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_REMARK, "Mean LOS velocity from SBAS (mm/yr)", Out)) die("could not set title","");
	if (GMT_Set_Comment (API, GMT_IS_GRID, GMT_COMMENT_IS_TITLE, "Mean Line-Of-Sight velocity from SBAS", Out)) die("could not set title","");
	if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_GRID_ALL, NULL, outfile, Out)) {
		die("Failed to write output grid",outfile);
	}

	Out->data = save_grid;	/* Restore the pointer before destroying */

	return(1);
}


int free_memory_ts(int N,float *phi,float *var,char **gfile,char **cfile,float *disp,double *G,double *A,double *Gs,int *H,double *d,double *ds,int *L,float *res,float *vel,double *time,int *flag,float *bperp,float *dem,double *work,int *jpvt,int *hit) {

	int i;

	for (i=0;i<N;i++) {
		free(gfile[i]);
		free(cfile[i]);
	}
	free(phi);
	free(var);
	free(gfile);
	free(cfile);
	free(disp);
	free(G);
	free(A);
	free(Gs);
	free(H);
	free(d);
	free(ds);
	free(L);
	free(res);
	free(vel);
	free(time);
	free(flag);
	free(bperp);
	free(dem);
	free(work);
	free(jpvt);
	free(hit);

	return(1);
}

int sum_intfs(float *phi, int *mark, float *screen, int xdim, int ydim, int N) {

	int n,i,j,sum=0;

	for (i=0;i<N;i++) sum+=abs(mark[i]);
	//fprintf(stderr,"%d sums       ",sum);

	for (i=0;i<ydim;i++) {
		for (j=0;j<xdim;j++) {
			screen[i*xdim+j] = 0.0;
		}
	}

	if (sum != 0) {
		for (n=0;n<N;n++) {
			if (mark[n] == 0) continue;
			for (i=0;i<ydim;i++) {
				for (j=0;j<xdim;j++) {
					screen[i*xdim+j] += -phi[n*xdim*ydim+i*xdim+j]*(float)mark[n]/(float)sum;
				}
			}
		}
	}

	return(1);

}

int connect(int *L, int *H, double *time, int *hit, int *mark, int N, int S, int n, int mode) {
	
	// mode = 0 for all connections, mode = 1 for even connections
	
	int i,j;//,sum = 0;
	for (i=0;i<N;i++) mark[i] = 0;

	for (i=0;i<S;i++) {
		if (hit[i*S+n] == 1) {
			for(j=0;j<N;j++) {
				if (L[i] == H[2*j] && L[n] == H[2*j+1]) mark[j] = -1;
			}
		}
		if (hit[n*S+i] == 1) {
			for(j=0;j<N;j++) {
				if (L[n] == H[2*j] && L[i] == H[2*j+1]) mark[j] = 1;
			}
		}
	}

	/* trim the result */
	if (mode == 1) {
		for (i=0;i<S;i++) { 
			if (hit[i*S+n] == 1) {
				for (j=n;j<S;j++) {
					if (hit[n*S+j] == 1 && fabs((time[n]-time[i]) - (time[j]-time[n])) < 5) break;
				}
				if (j == S) {
					for (j=0;j<N;j++) if (L[i] == H[2*j] && L[n] == H[2*j+1]) mark[j] = 0;
				}
			}
			if (hit[n*S+i] == 1) {
				for (j=0;j<n;j++) {
					if (hit[j*S+n] == 1 && fabs((time[n]-time[j]) - (time[i]-time[n])) < 5) break;
				}
				if (j == n) {
					for (j=0;j<N;j++) if (L[n] == H[2*j] && L[i] == H[2*j+1]) mark[j] = 0;
				}
			}
		}
	}

/*
	for (i=0;i<N;i++) sum += mark[i];
	while (sum != 0) {
		if (sum > 0) {
			j=N-1;
			while (j>=0 && mark[j] == 0) j--;
			if (j<0) die("incorrect process of trimming connection verctor","");
			mark[j] = 0;
			sum -= 1;
		}
		if (sum < 0) {
			j=0;
			while (j<N && mark[j] == 0) j++;
			if (j>=N) die("incorrect process of trimming connection verctor","");
			mark[j] = 0;
			sum += 1;
		}
 
	}
*/

	return(1);
}


double compute_noise(float *screen,int xdim, int ydim) {
	int i,j,n;
	double sum=0.0,rms=0.0;

	n=0;
	for (i=0;i<ydim;i++) {
		for (j=0;j<xdim;j++) {
			if(isnan(screen[i*xdim+j]) == 0) {
				sum = sum+(double)screen[i*xdim+j];
				n++;
			}
		}
	}
	
	if (sum != 0) {
		sum = sum/(double)n;

		for (i=0;i<ydim;i++) {
			for (j=0;j<xdim;j++) {
				if(isnan(screen[i*xdim+j]) == 0) {
					rms = rms+((double)screen[i*xdim+j]-sum)*((double)screen[i*xdim+j]-sum);
				}
			}
		}

		rms = sqrt(rms/(double)n);
	}
	//fprintf(stderr,"num points calculated: %d\n",n);	
	return(rms);

}


int apply_screen(float *screen, float *phi, int xdim, int ydim, int N, int *mark){
	// also correct for the ones not used in estimation of aps. 
	int i,j,n;

	for(n=0;n<N;n++) {
		if (mark[n] != 0) {
			//fprintf(stderr,"applying atm screen to intf %d...\n",n);
			for(i=0;i<ydim;i++) {
				for(j=0;j<xdim;j++) {
					phi[n*xdim*ydim+i*xdim+j] = phi[n*xdim*ydim+i*xdim+j] + screen[i*xdim+j]*mark[n];
				}
			}
		}
	}

	return(1);
}


int remove_ts(float *phi, float *ts, int xdim, int ydim, int N, int S, int *H, int *L) {

	int i,j,n,h1,h2;

	for (n=0;n<N;n++) {
		for (i=0;i<S;i++){
			if(H[2*n] == L[i]) h1 = i;
			if(H[2*n+1] == L[i]) h2 = i;
		}
		//fprintf(stderr,"removing deformation for pair %d_%d...\n",L[h1],L[h2]);
		for (i=0;i<ydim;i++) {
			for (j=0;j<xdim;j++){
				phi[n*xdim*ydim+i*xdim+j] = phi[n*xdim*ydim+i*xdim+j] - ts[h2*xdim*ydim+i*xdim+j] + ts[h1*xdim*ydim+i*xdim+j];
			}
		}
	}

	return(1);
}



int rank_double(double *nums, int *seq, int n) {

	// rank the numbers, seq will record the sequence
	int i,j,recn,*mark;
	double *nums2,rec;

	nums2 = (double *)malloc(n*sizeof(double));
	mark = (int *)malloc(n*sizeof(int));

	for (i=0;i<n;i++) nums2[i] = nums[i];
	for (i=0;i<n;i++) {
		if (nums2[i] == 0.0) mark[i]=1; 
		else mark[i]=0;
		//fprintf(stderr,"%d ",mark[i]);
	}
	//fprintf(stderr,"\n\n");

	for (i=0;i<n;i++) {
		rec = 0.0;
		for (j=0;j<n;j++) {
			if (rec<fabs(nums2[j])) {
				rec = fabs(nums2[j]);
				recn = j;
			}
		}
		if (rec != 0.0) {
			seq[i] = recn;
			nums2[recn] = 0.0;
		}
		else {
			break;
		}
	}
	
	recn = 0;
	for (i=0;i<n;i++) if (mark[i] != 0) recn++;
	for (i=0;i<recn;i++) {
		for (j=0;j<n;j++) {
			if (mark[j] != 0) {
				seq[n-recn+i] = j;
				mark[j] = 0;
				break;
			}
		}
	}

	return(1);
}
