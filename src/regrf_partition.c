/*******************************************************************
   Copyright (C) 2001-2015 Leo Breiman, Adele Cutler and Merck & Co., Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*******************************************************************/

#include <R.h>
#include "rf.h"
#include <Rinternals.h>
#include <stdbool.h>
#include <stdlib.h>

void simpleLinReg(int nsample, double *x, double *y, double *coef,
      double *mse, int *hasPred);

void regRF(double *x, double *y, int *xdim, int *sampsize,
     int *nthsize, int *nrnodes, int *nTree, int *mtry, int *imp,
     int *cat, int *maxcat, int *jprint, int *doProx, int *oobprox,
           int *biasCorr, double *yptr, double *errimp, double *impmat,
           double *impSD, double *prox, int *treeSize, int *nodestatus,
           int *lDaughter, int *rDaughter, double *avnode, int *mbest,
           double *upper, double *mse, int *keepf, int *replace,
           int *testdat, double *xts, int *nts, double *yts, int *labelts,
           double *yTestPred, double *proxts, double *msets, double *coef,
           int *nout, int *inbag, double* yb, double* xb,double* ytr, double*xtmp,
           double * resOOB, int* in, int* nodex, int * varUsed, int* nind, double * ytree, int* nodexts, int* oobpair);


void computeCov(double* err, double* cov, int nsample, int mdim){ 
    for(int m1=0; m1<mdim; m1++){
		for(int m2=0; m2<mdim; m2++){
			cov[m1+m2*mdim]=0;
			for(int s=0; s<nsample; s++){
				cov[m1+m2*mdim]+=err[m1+s*mdim]*err[m2+s*mdim];
			}
			cov[m1+m2*mdim]/=nsample;
        }
    }
}
			
	  

void regRFMultiRes2(double *x, int *xdim, int *sampsize,
     int *nthsize, int *nrnodes, int *nTree, int *mtry, int *imp,
     int *cat, int *maxcat, int *jprint, int *doProx, int *oobprox,
           int *biasCorr, double *yptrs, double *errimp, double *impmat,
           double *impSD, double *prox, int *treeSize, int *nodestatus,
           int *lDaughter, int *rDaughter, double *avnode, int *mbest,
           double *upper, double *mse, int *keepf, int *replace,
           int *testdat, double *xts, int *nts, double *yts, int *labelts,
           double *yTestPred, double *proxts, double *msets, double *coef,
           int *nout, int *inbag, int *subdim, int* sampleCount, double* yptrmtx, 
		   double* cov, int* noutAll, int* partition
          

           ) {
  int nsample = xdim[0];
  int mdim = xdim[1];
  int ninds[mdim];
  //int ninds_reverse[mdim];
  //int yflag[mdim];
  int ydimCount = mdim/ *partition + 1;
  int xdimCount=mdim - 2*ydimCount;
  //zeroInt(yflag, mdim);
  zeroDouble(cov,mdim*mdim);
  zeroInt(noutAll, mdim*mdim);


  double * yb        = (double *) S_alloc(*sampsize, sizeof(double));
  double *xb         = (double *) S_alloc(xdimCount * *sampsize, sizeof(double));
  double *ytr        = (double *) S_alloc(nsample, sizeof(double));
  double *xtmp       = (double *) S_alloc(nsample, sizeof(double));
  double * resOOB    = (double *) S_alloc(nsample, sizeof(double));

  int * in        = (int *) S_alloc(nsample, sizeof(int));
  int *nodex      = (int *) S_alloc(nsample, sizeof(int));
  int *varUsed    = (int *) S_alloc(xdimCount, sizeof(int));
  int *nind = *replace ? NULL : (int *) S_alloc(nsample, sizeof(int));

   
  double *ytree      = *testdat? (double *) S_alloc(*nts, sizeof(double)):NULL;
  int *nodexts    = *testdat?(int *) S_alloc(*nts, sizeof(int)):NULL;
  
  int *oobpair = (*doProx && *oobprox) ?
  (int *) S_alloc(nsample * nsample, sizeof(int)) : NULL;

   if(xdimCount > mdim-1 || xdimCount < *mtry) {
                       Rprintf("Not a valid partition/mtry!");
                       return;
                  }
              


  double ySelected[nsample];
  double yptr[nsample];
  int xdimSelected[2]={nsample,xdimCount};
  //int yind[2*ydimCount];
  int xind[xdimCount];
  
  //int* noutAll =(int*)S_alloc(mdim*mdim,sizeof(int));
  double* xSelected =(double*)S_alloc(xdimCount*nsample,sizeof(double));
  double* yerr =(double*)S_alloc(mdim*nsample,sizeof(double));

  double* cov_tmp =(double*)S_alloc(mdim*mdim,sizeof(double));

  
  /*select random variables as predictors and response variable. */
  /*
  int i=-1;
  
  while(true){
	  // iterate for *sampleCount times, 
	  //each time randomly shuffle index, partition and combination
      i++;
      bool needMoreIter=false;
	  for (int q=0;q<mdim*mdim;q++)
	  {
		  if (noutAll[q]==0) {
          needMoreIter=true;
          break;
        }
      }

      if(!needMoreIter && i>*sampleCount)
        break;
	*/
	for (int i=0;i<*sampleCount;i++){

      Rprintf("\n iter: %d\n", i);
      
	  //shuffle the index ninds
         for(int m=0; m< mdim;m++) ninds[m] = m; 
         if (mdim > 1) 
		{
        for (int m = 0; m < mdim - 1; m++) 
        {
         int j = m + rand() / (RAND_MAX / (mdim - m) + 1);
         int temp = ninds[j];
         ninds[j] = ninds[m];
         ninds[m] = temp;
        }
    }
   //for(int m=0; m< mdim;m++) Rprintf("newind: %d\n", ninds[m]); 

	  
	  //iterate for partition*(patition-1)/2 times.
	  //partition the index to parts, each time combine any of two parts.
	  for (int i=0;i<*partition-1;i++){
        for (int j=i+1;j<*partition;j++){
			if (j<*partition-1){
	   Rprintf("comb: %d,%d/n",i,j);
	   
	    int yind[2*ydimCount];
		zeroDouble(yptrmtx,nsample*mdim);
		zeroDouble(yerr,nsample*mdim);
      
	  //get yind (2*ydimCount) for possible response variables
	  for (int k=0;k<ydimCount;k++){
                yind[k] = k+ydimCount*i;
                //printf("%d ",yind[k]);
                yind[k+ydimCount] = k+ydimCount*j;
                //printf("%d ",yind[k+ydimCount]);
            }

	  //get xind (mdim - 2*ydimCount) for predictor variables
	  int s = 0;
      for (int m=0;m<mdim;m++){
		  for (int r=0;r<ydimCount*2;r++){
			  if (m==yind[r]){
				  m+=1;
				  }
			  }
		  xind[s]=m;
		  //printf("%d ",xind[s]);
		  s++;
          }
		  
	//GetxSelected
	for(int s = 0; s<nsample; s++)  
        for(int m = 0; m < xdimCount; m++) {
              xSelected[m + s*xdimCount] = x[ninds[xind[m]] + s*mdim];
          }
	
      for(int j=0; j< (ydimCount*2);j++){// select y
                    //yflag[ninds[j]]=1; //flag y
                    for(int n=0; n<nsample;n++) 
						ySelected[n]=x[ninds[yind[j]]+n*mdim];
                   // zeroDouble(yptr, nsample);
                       
                       regRF(xSelected, ySelected, xdimSelected, sampsize,
                         nthsize, nrnodes, nTree,mtry, imp,
                         cat,maxcat, jprint, doProx,oobprox,
                               biasCorr, yptr, errimp, impmat,
                              impSD, prox, treeSize,nodestatus,
                               lDaughter, rDaughter, avnode, mbest,
                              upper, mse, keepf, replace,
                               testdat, xts, nts,yts, labelts,
                               yTestPred, proxts, msets, coef,
                               nout, inbag,  yb,  xb,ytr, xtmp,
                              resOOB,  in,  nodex,  varUsed, nind,  ytree, nodexts, oobpair);

            for(int s=0; s<nsample; s++){
               yptrmtx[ninds[yind[j]]+s*mdim] = yptr[s];       
               yerr[ninds[yind[j]]+s*mdim] = yptrmtx[ninds[yind[j]]+s*mdim] - x[ninds[yind[j]]+s*mdim];                   
            }
    }
   
     zeroDouble(cov_tmp,mdim*mdim);
     computeCov(yerr,cov_tmp,nsample,mdim);
     for(int m1=0; m1<mdim; m1++){
        for(int m2=0; m2<mdim; m2++){
			if (cov_tmp[m1*mdim+m2]!=0)
            cov[m1*mdim+m2]=(cov[m1*mdim+m2]*noutAll[m1*mdim+m2]+cov_tmp[m1*mdim+m2])/(++noutAll[m1*mdim+m2]);
        }
     }


  }else{
  //j = *partition-1:
  Rprintf("comb: %d,%d/n",i,j);
  
		int yind[ydimCount + (mdim-j*ydimCount)];
		zeroDouble(yptrmtx,nsample*mdim);
		zeroDouble(yerr,nsample*mdim);
      
	  //get yind (2*ydimCount) for possible response variables
	  for (int k=0;k<ydimCount;k++)  yind[k] = k+ydimCount*i;
	  for (int k=ydimCount;k<ydimCount + (mdim-j*ydimCount);k++) yind[k] = ydimCount*(j-1)+k;  
	  //get xind (mdim - 2*ydimCount) for predictor variables
	  int s = 0;
	  if (s<xdimCount){
      for (int m=0;m<mdim;m++){
		  for (int r=0;r<ydimCount + (mdim-j*ydimCount);r++){
			  if (m==yind[r]){
				  m+=1;
				  }
			  }
		  xind[s]=m;
		  //Rprintf("%d,",xind[s]);
		  s++;
          }
	  }
		  
	//GetxSelected
	for(int s = 0; s<nsample; s++)  
        for(int m = 0; m < xdimCount; m++) {
              xSelected[m + s*xdimCount] = x[ninds[xind[m]] + s*mdim];
			  //Rprintf("%d,",ninds[xind[m]]);
          }
	
      for(int jj=0; jj<ydimCount + (mdim-j*ydimCount);jj++){// select y
                    //yflag[ninds[j]]=1; //flag y
                    for(int n=0; n<nsample;n++) 
						ySelected[n]=x[ninds[yind[jj]]+n*mdim];
					//Rprintf("%d,",ninds[yind[jj]]);
                   // zeroDouble(yptr, nsample);
                       
                       regRF(xSelected, ySelected, xdimSelected, sampsize,
                         nthsize, nrnodes, nTree,mtry, imp,
                         cat,maxcat, jprint, doProx,oobprox,
                               biasCorr, yptr, errimp, impmat,
                              impSD, prox, treeSize,nodestatus,
                               lDaughter, rDaughter, avnode, mbest,
                              upper, mse, keepf, replace,
                               testdat, xts, nts,yts, labelts,
                               yTestPred, proxts, msets, coef,
                               nout, inbag,  yb,  xb,ytr, xtmp,
                              resOOB,  in,  nodex,  varUsed, nind,  ytree, nodexts, oobpair);

            for(int s=0; s<nsample; s++){
               yptrmtx[ninds[yind[jj]]+s*mdim] = yptr[s];       
               yerr[ninds[yind[jj]]+s*mdim] = yptrmtx[ninds[yind[jj]]+s*mdim] - x[ninds[yind[jj]]+s*mdim];                   
            }
    }
   
     zeroDouble(cov_tmp,mdim*mdim);
     computeCov(yerr,cov_tmp,nsample,mdim);
     for(int m1=0; m1<mdim; m1++){
        for(int m2=0; m2<mdim; m2++){
			if (cov_tmp[m1*mdim+m2]!=0)
            cov[m1*mdim+m2]=(cov[m1*mdim+m2]*noutAll[m1*mdim+m2]+cov_tmp[m1*mdim+m2])/(++noutAll[m1*mdim+m2]);
        }
     }

  }
			
	}
	}
}
}



void regRF(double *x, double *y, int *xdim, int *sampsize,
	   int *nthsize, int *nrnodes, int *nTree, int *mtry, int *imp,
	   int *cat, int *maxcat, int *jprint, int *doProx, int *oobprox,
           int *biasCorr, double *yptr, double *errimp, double *impmat,
           double *impSD, double *prox, int *treeSize, int *nodestatus,
           int *lDaughter, int *rDaughter, double *avnode, int *mbest,
           double *upper, double *mse, int *keepf, int *replace,
           int *testdat, double *xts, int *nts, double *yts, int *labelts,
           double *yTestPred, double *proxts, double *msets, double *coef,
           int *nout, int *inbag, double* yb, double* xb,double* ytr, double*xtmp,
           double * resOOB, int* in, int* nodex, int * varUsed, int* nind, double * ytree, int* nodexts, int* oobpair
           )

            {
    /*************************************************************************
   Input:
   mdim=number of variables in data set
   nsample=number of cases

   nthsize=number of cases in a node below which the tree will not split,
   setting nthsize=5 generally gives good results.

   nTree=number of trees in run.  200-500 gives pretty good results

   mtry=number of variables to pick to split on at each node.  mdim/3
   seems to give genrally good performance, but it can be
   altered up or down

   imp=1 turns on variable importance.  This is computed for the
   mth variable as the percent rise in the test set mean sum-of-
   squared errors when the mth variable is randomly permuted.

  *************************************************************************/

    double errts = 0.0, averrb, meanY, meanYts, varY, varYts, r, xrand,
	errb = 0.0, resid=0.0, ooberr, ooberrperm, delta ;

    double   *tgini;

    int k, m, mr, n, nOOB, j, jout, idx, ntest, last, ktmp, nPerm,
        nsample, mdim, keepF, keepInbag;
    int  varImp, localImp;

  

    nsample = xdim[0];
    mdim = xdim[1];
    ntest = *nts;
    varImp = imp[0];
    localImp = imp[1];
    nPerm = imp[2];
    keepF = keepf[0];
    keepInbag = keepf[1];

    if (*jprint == 0) *jprint = *nTree + 1;



    /* If variable importance is requested, tgini points to the second
       "column" of errimp, otherwise it's just the same as errimp. */
    tgini = varImp ? errimp + mdim : errimp;

    averrb = 0.0;
    meanY = 0.0;
    varY = 0.0;

    zeroDouble(yptr, nsample);
    zeroInt(nout, nsample);
    for (n = 0; n < nsample; ++n) {
	varY += n * (y[n] - meanY)*(y[n] - meanY) / (n + 1);
	meanY = (n * meanY + y[n]) / (n + 1);
    }
    varY /= nsample;

    varYts = 0.0;
    meanYts = 0.0;
    if (*testdat) {
	for (n = 0; n < ntest; ++n) {
	    varYts += n * (yts[n] - meanYts)*(yts[n] - meanYts) / (n + 1);
	    meanYts = (n * meanYts + yts[n]) / (n + 1);
	}
	varYts /= ntest;
    }

    if (*doProx) {
        zeroDouble(prox, nsample * nsample);
	if (*testdat) zeroDouble(proxts, ntest * (nsample + ntest));
    }

    if (varImp) {
        zeroDouble(errimp, mdim * 2);
	if (localImp) zeroDouble(impmat, nsample * mdim);
    } else {
        zeroDouble(errimp, mdim);
    }
    if (*labelts) zeroDouble(yTestPred, ntest);

    /* print header for running output */
    if (*jprint <= *nTree) {
	Rprintf("     |      Out-of-bag   ");
	if (*testdat) Rprintf("|       Test set    ");
	Rprintf("|\n");
	Rprintf("Tree |      MSE  %%Var(y) ");
	if (*testdat) Rprintf("|      MSE  %%Var(y) ");
	Rprintf("|\n");
    }
    GetRNGstate();
    /*************************************
     * Start the loop over trees.
     *************************************/
    //for (j = 0; j < *nTree; ++j) 
    j=-1;
    while(true)
    {
      j++;
      bool needMoreTrees=false;
      for(int s=0;s<nsample;s++){
        if(yptr[s]==0){
           needMoreTrees=true;
           break;
        }
      }
      if( !needMoreTrees && j>=*nTree) break;


		idx = keepF ? j * *nrnodes : 0;
		zeroInt(in, nsample);
        zeroInt(varUsed, mdim);


        /* Draw a random sample for growing a tree. */
		if (*replace) { /* sampling with replacement */
			for (n = 0; n < *sampsize; ++n) {
				xrand = unif_rand();
				k = xrand * nsample;
				in[k] += 1;
				yb[n] = y[k];
				for(m = 0; m < mdim; ++m) {
					xb[m + n * mdim] = x[m + k * mdim];
				}
			}
		} else { /* sampling w/o replacement */
			for (n = 0; n < nsample; ++n) nind[n] = n;
			last = nsample - 1;
			for (n = 0; n < *sampsize; ++n) {
				ktmp = (int) (unif_rand() * (last+1));
                k = nind[ktmp];
                swapInt(nind[ktmp], nind[last]);
				last--;
				in[k] += 1;
				yb[n] = y[k];
				for(m = 0; m < mdim; ++m) {
					xb[m + n * mdim] = x[m + k * mdim];
				}
			}
		}
		if (keepInbag) {
			for (n = 0; n < nsample; ++n) inbag[n + j * nsample] = in[n];
		}
        /* grow the regression tree */
		regTree(xb, yb, mdim, *sampsize, lDaughter + idx, rDaughter + idx,
                upper + idx, avnode + idx, nodestatus + idx, *nrnodes,
                treeSize + j, *nthsize, *mtry, mbest + idx, cat, tgini,
                varUsed);
        /* predict the OOB data with the current tree */
		/* ytr is the prediction on OOB data by the current tree */
		predictRegTree(x, nsample, mdim, lDaughter + idx,
                       rDaughter + idx, nodestatus + idx, ytr, upper + idx,
                       avnode + idx, mbest + idx, treeSize[j], cat, *maxcat,
                       nodex);
		/* yptr is the aggregated prediction by all trees grown so far */
		
		errb = 0.0;
		ooberr = 0.0;
		jout = 0; /* jout is the number of cases that has been OOB so far */
		nOOB = 0; /* nOOB is the number of OOB samples for this tree */
		for (n = 0; n < nsample; ++n) {
			if (in[n] == 0) {
				nout[n]++;
                nOOB++;
				yptr[n] = ((nout[n]-1) * yptr[n] + ytr[n]) / nout[n];
				resOOB[n] = ytr[n] - y[n];
                ooberr += resOOB[n] * resOOB[n];
			}
            if (nout[n]) {
				jout++;
				errb += (y[n] - yptr[n]) * (y[n] - yptr[n]);
			}
		}
		errb /= jout;
    }
    PutRNGstate();
    /* end of tree iterations=======================================*/
}

/*----------------------------------------------------------------------*/
void regForest(double *x, double *ypred, int *mdim, int *n,
               int *ntree, int *lDaughter, int *rDaughter,
               int *nodestatus, int *nrnodes, double *xsplit,
               double *avnodes, int *mbest, int *treeSize, int *cat,
               int *maxcat, int *keepPred, double *allpred, int *doProx,
               double *proxMat, int *nodes, int *nodex) {
    int i, j, idx1, idx2, *junk;
    double *ytree;

    junk = NULL;
    ytree = (double *) S_alloc(*n, sizeof(double));
    if (*nodes) {
	zeroInt(nodex, *n * *ntree);
    } else {
	zeroInt(nodex, *n);
    }
    if (*doProx) zeroDouble(proxMat, *n * *n);
    if (*keepPred) zeroDouble(allpred, *n * *ntree);
    idx1 = 0;
    idx2 = 0;
    for (i = 0; i < *ntree; ++i) {
	zeroDouble(ytree, *n);
	predictRegTree(x, *n, *mdim, lDaughter + idx1, rDaughter + idx1,
                       nodestatus + idx1, ytree, xsplit + idx1,
                       avnodes + idx1, mbest + idx1, treeSize[i], cat, *maxcat,
                       nodex + idx2);

	for (j = 0; j < *n; ++j) ypred[j] += ytree[j];
	if (*keepPred) {
	    for (j = 0; j < *n; ++j) allpred[j + i * *n] = ytree[j];
	}
	/* if desired, do proximities for this round */
	if (*doProx) computeProximity(proxMat, 0, nodex + idx2, junk,
				      junk, *n);
	idx1 += *nrnodes; /* increment the offset */
	if (*nodes) idx2 += *n;
    }
    for (i = 0; i < *n; ++i) ypred[i] /= *ntree;
    if (*doProx) {
	for (i = 0; i < *n; ++i) {
	    for (j = i + 1; j < *n; ++j) {
                proxMat[i + j * *n] /= *ntree;
		proxMat[j + i * *n] = proxMat[i + j * *n];
	    }
	    proxMat[i + i * *n] = 1.0;
	}
    }
}

void simpleLinReg(int nsample, double *x, double *y, double *coef,
		  double *mse, int *hasPred) {
/* Compute simple linear regression of y on x, returning the coefficients,
   the average squared residual, and the predicted values (overwriting y). */
    int i, nout = 0;
    double sxx=0.0, sxy=0.0, xbar=0.0, ybar=0.0;
    double dx = 0.0, dy = 0.0, py=0.0;

    for (i = 0; i < nsample; ++i) {
	if (hasPred[i]) {
	    nout++;
	    xbar += x[i];
	    ybar += y[i];
	}
    }
    xbar /= nout;
    ybar /= nout;

    for (i = 0; i < nsample; ++i) {
	if (hasPred[i]) {
	    dx = x[i] - xbar;
	    dy = y[i] - ybar;
	    sxx += dx * dx;
	    sxy += dx * dy;
	}
    }
    coef[1] = sxy / sxx;
    coef[0] = ybar - coef[1] * xbar;

    *mse = 0.0;
    for (i = 0; i < nsample; ++i) {
	if (hasPred[i]) {
            py = coef[0] + coef[1] * x[i];
	    dy = y[i] - py;
	    *mse += dy * dy;
            /* y[i] = py; */
	}
    }
    *mse /= nout;
    return;
}
