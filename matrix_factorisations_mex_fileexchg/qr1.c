/*
 * QR factorization
 *
 * R = qr1(A)
 * [Q,R] = qr1(A)
 * [Q,R] = qr1(A,'pos')
 * [Q,R] = qr1(A,0)
 * [Q,R] = qr1(A,0,'pos')
 * [Q,R,E] = qr1(A) or [Q,R,E] = qr1(A,'matrix')
 * [Q,R,e] = qr1(A,'vector')
 * [Q,R,e] = qr1(A,0)
 *
 * example compile command (see also make_factor.m):
 * mex -O qr1.c libmwlapack.lib
 * or
 * mex -O qr1.c libmwblas.lib libmwlapack.lib (>= R2007B)
 *
 * calls the SGEQP3/DGEQP3/CGEQP3/ZGEQP3, SGEQRF/DGEQRF/CGEQRF/ZGEQRF,
 * SGEQRFP/DGEQRFP/CGEQRFP/ZGEQRFP and SORGQR/DORGQR/CUNGQR/ZUNGQR 
 * named LAPACK functions
 *
 * Ivo Houtzager
 * San Diego, 2015
 */

#include "mex.h"
#include "factor.h"
#include "matrix.h"

void qr_double(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    ptrdiff_t *Jp, lwork, info = 1;
    double *Jpr, *Qpr, *Rpr, *Ipr, *Qpi, *Rpi, *Ipi, *Ap;
    double *ptau, *pwork, *pwork2, *rwork, *psize, *psize2;
    size_t m, n, min_mn, n2, element_size = sizeof(double);
    size_t econ = 0, cplx = 0, perm = 0, vector = 0, positive = 0, dc = 1;
    mwIndex i, j, limit;
    mxClassID classid = mxDOUBLE_CLASS;
    mxComplexity cplxflag = mxREAL;
    
    /* check permutations */
    if (nlhs == 3) {
        perm = 1;
    }
    
    /* check complex */
    if (mxIsComplex(prhs[0])) {
        cplxflag = mxCOMPLEX;
        cplx = 1;
        dc = 2;
    }
    
    /* get matrix data */
    m = mxGetM(prhs[0]);
    n = mxGetN(prhs[0]);
    if (m == 0 || n == 0) {
        if (nlhs == 1) {
            plhs[0] = mxCreateNumericMatrix(m,n,classid,cplxflag);
        }
        else {
            plhs[1] = mxCreateNumericMatrix(m,n,classid,cplxflag);
            plhs[0] = mxCreateNumericMatrix(m,m,classid,cplxflag);
            if (m != 0) {
                Qpr = mxGetData(plhs[0]);
                for (j=0; j<m; j++) {
                    Qpr[j*m + j] = 1;
                }
            }
        }
        return;
    }
    if (nrhs >= 2) {
        if (mxIsChar(prhs[1])) {
            char vec[] = "vector";
            char pos[] = "pos";
            char *str = mxArrayToString(prhs[1]);
            if (strcmp(str,vec) == 0) {
                vector = 1;
            }
            if (strcmp(str,pos) == 0) {
                positive = 1;
            }
            mxFree(str);
        }
        else {
            if (mxGetScalar(prhs[1]) == 0) {
                econ = 1;
                vector = 1;
            }
            if (nrhs == 3) {
                if (mxIsChar(prhs[2])) {
                    char pos[] = "pos";
                    char *str = mxArrayToString(prhs[2]);
                    if (strcmp(str,pos) == 0) {
                       positive = 1;
                    }
                    mxFree(str);
                }
            }
        }
    }
    
    /* allocate tau */
    min_mn = min(m,n);
    ptau = mxMalloc(dc*min_mn*element_size);

    /* allocate A matrix */
    if (m > n && (econ != 1)) {
        Ap = mxMalloc(dc*m*m*element_size);
    }
    else {
        Ap = mxMalloc(dc*m*n*element_size);
    }
    
    /* copy input to A matrix */
    if (cplx) {
        Ipr = mxGetData(prhs[0]);
        Ipi = mxGetImagData(prhs[0]);
        for (i=0; i<m; i++) {
            for (j=0; j<n; j++) {
                Ap[j*2*m+2*i] = Ipr[j*m+i];
                Ap[j*2*m+2*i+1] = Ipi[j*m+i];
            }
        }
    }
    else {
        Ipr = mxGetData(prhs[0]);
        for (i=0; i<m; i++) {
            for (j=0; j<n; j++) {
                Ap[j*m+i] = Ipr[j*m+i];
            }
        }
    }
    
    /* determine blocksize */
    lwork = -1;
    psize = mxMalloc(dc*element_size);
    if (perm) {
        Jp = mxMalloc(n*sizeof(ptrdiff_t));
        if (cplx) {
            rwork = mxMalloc(2*n*element_size);
            zgeqp3(&m, &n, Ap, &m, Jp, ptau, psize, &lwork, rwork, &info);
        }
        else {
            dgeqp3(&m, &n, Ap, &m, Jp, ptau, psize, &lwork, &info);
        }
    }
    else if (positive) {
        if (cplx) {
            zgeqrfp(&m, &n, Ap, &m, ptau, psize, &lwork, &info);
        }
        else {
            dgeqrfp(&m, &n, Ap, &m, ptau, psize, &lwork, &info);
        }
    }
    else {
        if (cplx) {
            zgeqrf(&m, &n, Ap, &m, ptau, psize, &lwork, &info);
        }
        else {
            dgeqrf(&m, &n, Ap, &m, ptau, psize, &lwork, &info);
        }
    }
    lwork = psize[0];
    mxFree(psize);
    
    /* allocate workspace */
    pwork = mxMalloc(dc*lwork*element_size);

    /* calls the DGEQRF/DGEQP3 function */
    if (perm) {
        if (cplx) {
            zgeqp3(&m, &n, Ap, &m, Jp, ptau, pwork, &lwork, rwork, &info);
            mxFree(rwork);
        }
        else {
            dgeqp3(&m, &n, Ap, &m, Jp, ptau, pwork, &lwork, &info);
        }
    }
    else if (positive) {
        if (cplx) {
            zgeqrfp(&m, &n, Ap, &m, ptau, pwork, &lwork, &info);
        }
        else {
            dgeqrfp(&m, &n, Ap, &m, ptau, pwork, &lwork, &info);
        }
    }
    else {
        if (cplx) {
            zgeqrf(&m, &n, Ap, &m, ptau, pwork, &lwork, &info);
        }
        else {
            dgeqrf(&m, &n, Ap, &m, ptau, pwork, &lwork, &info);
        }
    }
    mxFree(pwork);
    if (info != 0) {
        mxFree(ptau);
        mxFree(Ap);
        if (perm) {
            mxFree(Jp);
            if (cplx) {
                mexErrMsgTxt("ZGEQP3 not successful");
            }
            else {
                mexErrMsgTxt("DGEQP3 not successful");
            }
        }
        else if (positive) {
            if (cplx) {
                mexErrMsgTxt("ZGEQRFP not successful");
            }
            else {
                mexErrMsgTxt("DGEQRFP not successful");
            }
        }
        else {
            if (cplx) {
                mexErrMsgTxt("ZGEQRF not successful");
            }
            else {
                mexErrMsgTxt("DGEQRF not successful");
            }
        }
    }

    /* extract lower triangular part */
    if ((econ == 1) && m > n) {
        if (nlhs == 1) {
            plhs[0] = mxCreateNumericMatrix(n,n,classid,cplxflag);
            if (cplx) {
                Rpr = mxGetData(plhs[0]);
                Rpi = mxGetImagData(plhs[0]);
            }
            else {
                Rpr = mxGetData(plhs[0]);
            }
        }
        else {
            plhs[1] = mxCreateNumericMatrix(n,n,classid,cplxflag);
            if (cplx) {
                Rpr = mxGetData(plhs[1]);
                Rpi = mxGetImagData(plhs[1]);
            }
            else {
                Rpr = mxGetData(plhs[1]);
            }
        }
        if (cplx) {
            for (j=0; j<n; j++) {
                limit = j < min_mn-1 ? j : min_mn-1;
                for (i=0; i<=limit; i++) {
                    Rpr[j*n+i] = Ap[2*j*m+2*i];
                    Rpi[j*n+i] = Ap[2*j*m+2*i+1];
                }
            }
        }
        else {
            for (j=0; j<n; j++) {
                limit = j < min_mn-1 ? j : min_mn-1;
                for (i=0; i<=limit; i++) {
                    Rpr[j*n+i] = Ap[j*m+i];
                }
            }
        }
    }
    else {
        if (nlhs == 1) {
            plhs[0] = mxCreateNumericMatrix(m,n,classid,cplxflag);
            if (cplx) {
                Rpr = mxGetData(plhs[0]);
                Rpi = mxGetImagData(plhs[0]);
            }
            else {
                Rpr = mxGetData(plhs[0]);
            }
        }
        else {
            plhs[1] = mxCreateNumericMatrix(m,n,classid,cplxflag);
            if (cplx) {
                Rpr = mxGetData(plhs[1]);
                Rpi = mxGetImagData(plhs[1]);
            }
            else {
                Rpr = mxGetData(plhs[1]);
            }
        }       
        if (cplx) {
            for (j=0; j<n; j++) {
                limit = j < min_mn-1 ? j : min_mn-1;
                for (i=0; i<=limit; i++) {
                    Rpr[j*m+i] = Ap[2*j*m+2*i];
                    Rpi[j*m+i] = Ap[2*j*m+2*i+1];
                }
            }
        }
        else {
            for (j=0; j<n; j++) {
                limit = j < min_mn-1 ? j : min_mn-1;
                for (i=0; i<=limit; i++) {
                    Rpr[j*m+i] = Ap[j*m+i];
                }
            }
        }
    }
    
    if (perm) {
        if (vector) {
            plhs[2] = mxCreateNumericMatrix(n,1,classid,mxREAL);
            Jpr = mxGetData(plhs[2]);
            for (i=0; i<n; i++) {
                Jpr[i] = (double)Jp[i];
            }
        }      
        else {
            plhs[2] = mxCreateNumericMatrix(n,n,classid,mxREAL);
            Jpr = mxGetData(plhs[2]);
            for (i=0; i<n; i++) {
                size_t idx = Jp[i]-1;
                Jpr[i*n+idx] = 1;
            }
        }
        mxFree(Jp);
    }
    
    if (nlhs >= 2) {
        n2 = (econ == 1) ? min_mn : m;

        /* determine blocksize */
        lwork = -1;
        psize2 = mxMalloc(dc*element_size);
        if (cplx) {
            zungqr(&m, &n2, &min_mn, Ap, &m, ptau, psize2, &lwork, &info);
        }
        else {
            dorgqr(&m, &n2, &min_mn, Ap, &m, ptau, psize2, &lwork, &info);
        }
        lwork = psize2[0];
        mxFree(psize2);

        /* allocate workspace */
        pwork2 = mxMalloc(dc*lwork*element_size);

        /* calls the DORGQR function */
        if (cplx) {
            zungqr(&m, &n2, &min_mn, Ap, &m, ptau, pwork2, &lwork, &info);
        }
        else {
            dorgqr(&m, &n2, &min_mn, Ap, &m, ptau, pwork2, &lwork, &info);
        }
        mxFree(pwork2);
        if (info != 0) {
            mxFree(ptau);
            mxFree(Ap);
            if (cplx) {
                mexErrMsgTxt("DUNGQR not successful");
            }
            else {
                mexErrMsgTxt("DORGQR not successful");
            }
        }

        /* allocate Q matrix */
        plhs[0] = mxCreateNumericMatrix(m,n2,classid,cplxflag);
        if (cplx) {
            Qpr = mxGetData(plhs[0]);
            Qpi = mxGetImagData(plhs[0]);

            /* copy Ap to Qp */
            for (i=0; i<m; i++) {
                for (j=0; j<n2; j++) {
                    Qpr[j*m+i] = Ap[j*2*m+2*i];
                    Qpi[j*m+i] = Ap[j*2*m+2*i+1];
                }
            }
        }
        else {
            Qpr = mxGetData(plhs[0]);

            /* copy Ap to Qp */
            for (i=0; i<m; i++) {
                for (j=0; j<n2; j++) {
                    Qpr[j*m+i] = Ap[j*m+i];
                }
            }
        }
    }
    
    mxFree(ptau);
    mxFree(Ap);
}

void qr_single(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    ptrdiff_t *Jp, lwork, info = 1;
    float *Jpr, *Qpr, *Rpr, *Ipr, *Qpi, *Rpi, *Ipi, *Ap;
    float *ptau, *pwork, *pwork2, *rwork, *psize, *psize2;
    size_t m, n, min_mn, n2, element_size = sizeof(float);
    size_t econ = 0, cplx = 0, perm = 0, vector = 0, positive = 0, dc = 1;
    mwIndex i, j, limit;
    mxClassID classid = mxSINGLE_CLASS;
    mxComplexity cplxflag = mxREAL;
    
    /* check permutations */
    if (nlhs == 3) {
        perm = 1;
    }
    
    /* check complex */
    if (mxIsComplex(prhs[0])) {
        cplxflag = mxCOMPLEX;
        cplx = 1;
        dc = 2;
    }
    
    /* get matrix data */
    m = mxGetM(prhs[0]);
    n = mxGetN(prhs[0]);
    if (m == 0 || n == 0) {
        if (nlhs == 1) {
            plhs[0] = mxCreateNumericMatrix(m,n,classid,cplxflag);
        }
        else {
            plhs[1] = mxCreateNumericMatrix(m,n,classid,cplxflag);
            plhs[0] = mxCreateNumericMatrix(m,m,classid,cplxflag);
            if (m != 0) {
                Qpr = mxGetData(plhs[0]);
                for (j=0; j<m; j++) {
                    Qpr[j*m + j] = 1;
                }
            }
        }
        return;
    }
    if (nrhs >= 2) {
        if (mxIsChar(prhs[1])) {
            char vec[] = "vector";
            char pos[] = "pos";
            char *str = mxArrayToString(prhs[1]);
            if (strcmp(str,vec) == 0) {
                vector = 1;
            }
            if (strcmp(str,pos) == 0) {
                positive = 1;
            }
            mxFree(str);
        }
        else {
            if (mxGetScalar(prhs[1]) == 0) {
                econ = 1;
                vector = 1;
            }
            if (nrhs == 3) {
                if (mxIsChar(prhs[2])) {
                    char pos[] = "pos";
                    char *str = mxArrayToString(prhs[2]);
                    if (strcmp(str,pos) == 0) {
                       positive = 1;
                    }
                    mxFree(str);
                }
            }
        }
    }

    /* allocate tau */
    min_mn = min(m,n);
    ptau = mxMalloc(dc*min_mn*element_size);

    /* allocate A matrix */
    if (m > n && (econ != 1)) {
        Ap = mxMalloc(dc*m*m*element_size);
    }
    else {
        Ap = mxMalloc(dc*m*n*element_size);
    }
    
    /* copy input to A matrix */
    if (cplx) {
        Ipr = mxGetData(prhs[0]);
        Ipi = mxGetImagData(prhs[0]);
        for (i=0; i<m; i++) {
            for (j=0; j<n; j++) {
                Ap[j*2*m+2*i] = Ipr[j*m+i];
                Ap[j*2*m+2*i+1] = Ipi[j*m+i];
            }
        }
    }
    else {
        Ipr = mxGetData(prhs[0]);
        for (i=0; i<m; i++) {
            for (j=0; j<n; j++) {
                Ap[j*m+i] = Ipr[j*m+i];
            }
        }
    }
    
    /* determine blocksize */
    lwork = -1;
    psize = mxMalloc(dc*element_size);
    if (perm) {
        Jp = mxMalloc(n*sizeof(ptrdiff_t));
        if (cplx) {
            rwork = mxMalloc(2*n*element_size);
            cgeqp3(&m, &n, Ap, &m, Jp, ptau, psize, &lwork, rwork, &info);
        }
        else {
            sgeqp3(&m, &n, Ap, &m, Jp, ptau, psize, &lwork, &info);
        }
    }
    else if (positive) {
        if (cplx) {
            cgeqrfp(&m, &n, Ap, &m, ptau, psize, &lwork, &info);
        }
        else {
            sgeqrfp(&m, &n, Ap, &m, ptau, psize, &lwork, &info);
        }
    }
    else {
        if (cplx) {
            cgeqrf(&m, &n, Ap, &m, ptau, psize, &lwork, &info);
        }
        else {
            sgeqrf(&m, &n, Ap, &m, ptau, psize, &lwork, &info);
        }
    }
    lwork = psize[0];
    mxFree(psize);

    /* allocate workspace */
    pwork = mxMalloc(dc*lwork*element_size);

    /* calls the SGEQRF/SGEQP3 function */
    if (perm) {
        if (cplx) {
            cgeqp3(&m, &n, Ap, &m, Jp, ptau, pwork, &lwork, rwork, &info);
            mxFree(rwork);
        }
        else {
            sgeqp3(&m, &n, Ap, &m, Jp, ptau, pwork, &lwork, &info);
        }
    }
    else if (positive) {
        if (cplx) {
            cgeqrfp(&m, &n, Ap, &m, ptau, pwork, &lwork, &info);
        }
        else {
            sgeqrfp(&m, &n, Ap, &m, ptau, pwork, &lwork, &info);
        }
    }
    else {
        if (cplx) {
            cgeqrf(&m, &n, Ap, &m, ptau, pwork, &lwork, &info);
        }
        else {
            sgeqrf(&m, &n, Ap, &m, ptau, pwork, &lwork, &info);
        }
    }
    mxFree(pwork);
    if (info != 0) {
        mxFree(ptau);
        mxFree(Ap);
        if (perm) {
            mxFree(Jp);
            if (cplx) {
                mexErrMsgTxt("CGEQP3 not successful");
            }
            else {
                mexErrMsgTxt("SGEQP3 not successful");
            }
        }
        else if (positive) {
            if (cplx) {
                mexErrMsgTxt("CGEQRFP not successful");
            }
            else {
                mexErrMsgTxt("SGEQRFP not successful");
            }
        }
        else {
            if (cplx) {
                mexErrMsgTxt("CGEQRF not successful");
            }
            else {
                mexErrMsgTxt("SGEQRF not successful");
            }
        }
    }

    /* extract lower triangular part */
    if ((econ == 1) && m > n) {
        if (nlhs == 1) {
            plhs[0] = mxCreateNumericMatrix(n,n,classid,cplxflag);
            if (cplx) {
                Rpr = mxGetData(plhs[0]);
                Rpi = mxGetImagData(plhs[0]);
            }
            else {
                Rpr = mxGetData(plhs[0]);
            }
        }
        else {
            plhs[1] = mxCreateNumericMatrix(n,n,classid,cplxflag);
            if (cplx) {
                Rpr = mxGetData(plhs[1]);
                Rpi = mxGetImagData(plhs[1]);
            }
            else {
                Rpr = mxGetData(plhs[1]);
            }
        }
        if (cplx) {
            for (j=0; j<n; j++) {
                limit = j < min_mn-1 ? j : min_mn-1;
                for (i=0; i<=limit; i++) {
                    Rpr[j*n+i] = Ap[2*j*m+2*i];
                    Rpi[j*n+i] = Ap[2*j*m+2*i+1];
                }
            }
        }
        else {
            for (j=0; j<n; j++) {
                limit = j < min_mn-1 ? j : min_mn-1;
                for (i=0; i<=limit; i++) {
                    Rpr[j*n+i] = Ap[j*m+i];
                }
            }
        }
    }
    else {
        if (nlhs == 1) {
            plhs[0] = mxCreateNumericMatrix(m,n,classid,cplxflag);
            if (cplx) {
                Rpr = mxGetData(plhs[0]);
                Rpi = mxGetImagData(plhs[0]);
            }
            else {
                Rpr = mxGetData(plhs[0]);
            }
        }
        else {
            plhs[1] = mxCreateNumericMatrix(m,n,classid,cplxflag);
            if (cplx) {
                Rpr = mxGetData(plhs[1]);
                Rpi = mxGetImagData(plhs[1]);
            }
            else {
                Rpr = mxGetData(plhs[1]);
            }
        }       
        if (cplx) {
            for (j=0; j<n; j++) {
                limit = j < min_mn-1 ? j : min_mn-1;
                for (i=0; i<=limit; i++) {
                    Rpr[j*m+i] = Ap[2*j*m+2*i];
                    Rpi[j*m+i] = Ap[2*j*m+2*i+1];
                }
            }
        }
        else {
            for (j=0; j<n; j++) {
                limit = j < min_mn-1 ? j : min_mn-1;
                for (i=0; i<=limit; i++) {
                    Rpr[j*m+i] = Ap[j*m+i];
                }
            }
        }
    }

    if (perm) {
        if (vector) {
            plhs[2] = mxCreateNumericMatrix(n,1,classid,mxREAL);
            Jpr = mxGetData(plhs[2]);
            for (i=0; i<n; i++) {
                Jpr[i] = (float)Jp[i];
            }
        }      
        else {
            plhs[2] = mxCreateNumericMatrix(n,n,classid,mxREAL);
            Jpr = mxGetData(plhs[2]);
            for (i=0; i<n; i++) {
                size_t idx = Jp[i]-1;
                Jpr[i*n+idx] = 1;
            }
        }
        mxFree(Jp);
    }
    
    if (nlhs >= 2) {
        n2 = (econ == 1) ? min_mn : m;

        /* determine blocksize */
        lwork = -1;
        psize2 = mxMalloc(dc*element_size);
        if (cplx) {
            cungqr(&m, &n2, &min_mn, Ap, &m, ptau, psize2, &lwork, &info);
        }
        else {
            sorgqr(&m, &n2, &min_mn, Ap, &m, ptau, psize2, &lwork, &info);
        }
        lwork = psize2[0];
        mxFree(psize2);

        /* allocate workspace */
        pwork2 = mxMalloc(dc*lwork*element_size);

        /* calls the DORGQR function */
        if (cplx) {
            cungqr(&m, &n2, &min_mn, Ap, &m, ptau, pwork2, &lwork, &info);
        }
        else {
            sorgqr(&m, &n2, &min_mn, Ap, &m, ptau, pwork2, &lwork, &info);
        }
        mxFree(pwork2);
        if (info != 0) {
            mxFree(ptau);
            mxFree(Ap);
            if (cplx) {
                mexErrMsgTxt("CUNGQR not successful");
            }
            else {
                mexErrMsgTxt("SORGQR not successful");
            }
        }

        /* allocate Q matrix */
        plhs[0] = mxCreateNumericMatrix(m,n2,classid,cplxflag);
        if (cplx) {
            Qpr = mxGetData(plhs[0]);
            Qpi = mxGetImagData(plhs[0]);

            /* copy Ap to Qp */
            for (i=0; i<m; i++) {
                for (j=0; j<n2; j++) {
                    Qpr[j*m+i] = Ap[j*2*m+2*i];
                    Qpi[j*m+i] = Ap[j*2*m+2*i+1];
                }
            }
        }
        else {
            Qpr = mxGetData(plhs[0]);

            /* copy Ap to Qp */
            for (i=0; i<m; i++) {
                for (j=0; j<n2; j++) {
                    Qpr[j*m+i] = Ap[j*m+i];
                }
            }
        }
    }

    mxFree(ptau);
    mxFree(Ap);
}


void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    /* check for proper number of arguments */
    if (nrhs != 1 && nrhs != 2 && nrhs != 3) {
        mexErrMsgTxt("QR1 requires one, two, or three input arguments.");
    }
    if (nlhs > 3) {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (!mxIsNumeric(prhs[0]) || mxIsSparse(prhs[0])) {
        mexErrMsgTxt( "Input must be a full matrix." );
    }
    if (mxIsDouble(prhs[0])) {
        qr_double(nlhs, plhs, nrhs, prhs);
    }
    else if (mxIsSingle(prhs[0])) {
        qr_single(nlhs, plhs, nrhs, prhs);
    }
    else {
        mexErrMsgTxt( "Class is not supported." );
    }
}
