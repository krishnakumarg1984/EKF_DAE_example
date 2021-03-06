/*
 * LQ factorization
 *
 * L = lq(A)
 * [L,Q] = lq(A)
 * [L,Q] = lq(A,0)
 *
 * example compile command (see also make_factor.m):
 * mex -O lq.c libmwlapack.lib
 * or
 * mex -O lq.c libmwblas.lib libmwlapack.lib (>= R2007B)
 *
 * calls the SGELQF/DGELQF/CGELQF/ZGELQF and
 * SORGLQ/DORGLQ/CUNGLQ/ZUNGLQ named LAPACK functions
 *
 * Ivo Houtzager
 *
 * Delft Center of Systems and Control
 * The Netherlands, 2010
 */

#include "mex.h"
#include "factor.h"
#include "matrix.h"

void lq_double(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    ptrdiff_t lwork, info = 1;
    double *Qpr, *Qpi, *Lpr, *Lpi, *Ipr, *Ipi, *Ap, *ptau, *pwork, *pwork2, *psize, *psize2;
    size_t m, n, min_mn, lda, m2, element_size = sizeof(double), econ = 0, cplx = 0, dc = 1;
    mwIndex i, j, start;
    mxClassID classid = mxDOUBLE_CLASS;
    mxComplexity cplxflag = mxREAL;

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
            plhs[0] = mxCreateNumericMatrix(m,n,classid,cplxflag);
            plhs[1] = mxCreateNumericMatrix(n,n,classid,cplxflag);
            if (n != 0) {
                Qpr = mxGetData(plhs[1]);
                for (j=0; j<n; j++) {
                    Qpr[j*n + j] = 1;
                }
            }
        }
        return;
    }
    if (nrhs == 2) {
        if (mxGetScalar(prhs[1]) == 0) {
            econ = 1;
        }
    }

    /* allocate tau */
    min_mn = min(m,n);
    ptau = mxMalloc(dc*min_mn*element_size);

    /* allocate A matrix */
    if ((m < n) && (econ != 1)) {
        Ap = mxMalloc(dc*n*n*element_size);
        lda = n;
    }
    else {
        Ap = mxMalloc(dc*m*n*element_size);
        lda = m;
    }

    /* copy input to A matrix */
    if (cplx) {
        Ipr = mxGetData(prhs[0]);
        Ipi = mxGetImagData(prhs[0]);
        for (i=0; i<m; i++) {
            for (j=0; j<n; j++) {
                Ap[j*2*lda+2*i] = Ipr[j*m+i];
                Ap[j*2*lda+2*i+1] = Ipi[j*m+i];
            }
        }
    }
    else {
        Ipr = mxGetData(prhs[0]);
        for (i=0; i<m; i++) {
            for (j=0; j<n; j++) {
                Ap[j*lda+i] = Ipr[j*m+i];
            }
        }
    }

    /* determine blocksize */
    lwork = -1;
    psize = mxMalloc(dc*element_size);
    if (cplx) {
        zgelqf(&m, &n, Ap, &lda, ptau, psize, &lwork, &info);
    }
    else {
        dgelqf(&m, &n, Ap, &lda, ptau, psize, &lwork, &info);
    }
    lwork = psize[0];
    mxFree(psize);

    /* allocate workspace */
    pwork = mxMalloc(dc*lwork*element_size);

    /* calls the DGELQF function */
    if (cplx) {
        zgelqf(&m, &n, Ap, &lda, ptau, pwork, &lwork, &info);
    }
    else {
        dgelqf(&m, &n, Ap, &lda, ptau, pwork, &lwork, &info);
    }
    mxFree(pwork);
    if (info != 0) {
        mxFree(ptau);
        mxFree(Ap);
        if (cplx) {
            mexErrMsgTxt("ZGELQF not successful");
        }
        else {
            mexErrMsgTxt("DGELQF not successful");
        }
    }

    /* extract lower triangular part */
    if ((econ == 1) && m < n) {
        plhs[0] = mxCreateNumericMatrix(m,m,classid,cplxflag);
        if (cplx) {
            Lpr = mxGetData(plhs[0]);
            Lpi = mxGetImagData(plhs[0]);
            for (j=0; j<m; j++) {
                start = j<m ? j:m;
                for (i=start; i<m; i++) {
                    Lpr[j*m+i] = Ap[j*2*lda+2*i];
                    Lpi[j*m+i] = Ap[j*2*lda+2*i+1];
                }
            }
        }
        else {
            Lpr = mxGetData(plhs[0]);
            for (j=0; j<m; j++) {
                start = j<m ? j:m;
                for (i=start; i<m; i++) {
                    Lpr[j*m+i] = Ap[j*lda+i];
                }
            }
        }
    }
    else {
        plhs[0] = mxCreateNumericMatrix(m,n,classid,cplxflag);
        if (cplx) {
            Lpr = mxGetData(plhs[0]);
            Lpi = mxGetImagData(plhs[0]);
            for (j=0; j<n; j++) {
                start = j<min_mn ? j:min_mn;
                for (i=start; i<m; i++) {
                    Lpr[j*m+i] = Ap[j*2*lda+2*i];
                    Lpi[j*m+i] = Ap[j*2*lda+2*i+1];
                }
            }
        }
        else {
            Lpr = mxGetData(plhs[0]);
            for (j=0; j<n; j++) {
                start = j<min_mn ? j:min_mn;
                for (i=start; i<m; i++) {
                    Lpr[j*m+i] = Ap[j*lda+i];
                }
            }
        }
    }

    if (nlhs == 2) {
        m2 = (econ == 1) ? min_mn : n;

        /* determine blocksize */
        lwork = -1;
        psize2 = mxMalloc(dc*element_size);
        if (cplx) {
            zunglq(&m2, &n, &min_mn, Ap, &lda, ptau, psize2, &lwork, &info);
        }
        else {
            dorglq(&m2, &n, &min_mn, Ap, &lda, ptau, psize2, &lwork, &info);
        }
        lwork = psize2[0];
        mxFree(psize2);

        /* allocate workspace */
        pwork2 = mxMalloc(dc*lwork*element_size);

        /* calls the DORGLQ function */
        if (cplx) {
            zunglq(&m2, &n, &min_mn, Ap, &lda, ptau, pwork2, &lwork, &info);
        }
        else {
            dorglq(&m2, &n, &min_mn, Ap, &lda, ptau, pwork2, &lwork, &info);
        }
        mxFree(pwork2);
        if (info != 0) {
            mxFree(ptau);
            mxFree(Ap);
            if (cplx) {
                mexErrMsgTxt("ZUNGLQ not successful");
            }
            else {
                mexErrMsgTxt("DORGLQ not successful");
            }
        }

        /* allocate Q matrix */
        plhs[1] = mxCreateNumericMatrix(m2,n,classid,cplxflag);
        if (cplx) {
            Qpr = mxGetData(plhs[1]);
            Qpi = mxGetImagData(plhs[1]);

            /* copy Ap to Qp */
            for (i=0; i<m2; i++) {
                for (j=0; j<n; j++) {
                    Qpr[j*m2+i] = Ap[j*2*lda+2*i];
                    Qpi[j*m2+i] = Ap[j*2*lda+2*i+1];
                }
            }
        }
        else {
            Qpr = mxGetData(plhs[1]);

            /* copy Ap to Qp */
            for (i=0; i<m2; i++) {
                for (j=0; j<n; j++) {
                    Qpr[j*m2+i] = Ap[j*lda+i];
                }
            }
        }
    }

    mxFree(ptau);
    mxFree(Ap);
}


void lq_single(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    ptrdiff_t lwork, info = 1;
    float *Qpr, *Qpi, *Lpr, *Lpi, *Ipr, *Ipi, *Ap, *ptau, *pwork, *pwork2, *psize, *psize2;
    size_t m, n, min_mn, lda, m2, element_size = sizeof(float), econ = 0, cplx = 0, dc = 1;
    mwIndex i, j, start;
    mxClassID classid = mxSINGLE_CLASS;
    mxComplexity cplxflag = mxREAL;

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
            plhs[0] = mxCreateNumericMatrix(m,n,classid,cplxflag);
            plhs[1] = mxCreateNumericMatrix(n,n,classid,cplxflag);
            if (n != 0) {
                Qpr = mxGetData(plhs[1]);
                for (j=0; j<n; j++) {
                    Qpr[j*n + j] = 1;
                }
            }
        }
        return;
    }
    if (nrhs == 2) {
        if (mxGetScalar(prhs[1]) == 0) {
            econ = 1;
        }
    }

    /* allocate tau */
    min_mn = min(m,n);
    ptau = mxMalloc(dc*min_mn*element_size);

    /* allocate A matrix */
    if ((m < n) && (econ != 1)) {
        Ap = mxMalloc(dc*n*n*element_size);
        lda = n;
    }
    else {
        Ap = mxMalloc(dc*m*n*element_size);
        lda = m;
    }

    /* copy input to A matrix */
    if (cplx) {
        Ipr = mxGetData(prhs[0]);
        Ipi = mxGetImagData(prhs[0]);
        for (i=0; i<m; i++) {
            for (j=0; j<n; j++) {
                Ap[j*2*lda+2*i] = Ipr[j*m+i];
                Ap[j*2*lda+2*i+1] = Ipi[j*m+i];
            }
        }
    }
    else {
        Ipr = mxGetData(prhs[0]);
        for (i=0; i<m; i++) {
            for (j=0; j<n; j++) {
                Ap[j*lda+i] = Ipr[j*m+i];
            }
        }
    }

    /* determine blocksize */
    lwork = -1;
    psize = mxMalloc(dc*element_size);
    if (cplx) {
        cgelqf(&m, &n, Ap, &lda, ptau, psize, &lwork, &info);
    }
    else {
        sgelqf(&m, &n, Ap, &lda, ptau, psize, &lwork, &info);
    }
    lwork = psize[0];
    mxFree(psize);

    /* allocate workspace */
    pwork = mxMalloc(dc*lwork*element_size);

    /* calls the SGELQF function */
    if (cplx) {
        cgelqf(&m, &n, Ap, &lda, ptau, pwork, &lwork, &info);
    }
    else {
        sgelqf(&m, &n, Ap, &lda, ptau, pwork, &lwork, &info);
    }
    mxFree(pwork);
    if (info != 0) {
        mxFree(ptau);
        mxFree(Ap);
        if (cplx) {
            mexErrMsgTxt("CGELQF not successful");
        }
        else {
            mexErrMsgTxt("SGELQF not successful");
        }
    }

    /* extract lower triangular part */
    if ((econ == 1) && m < n) {
        plhs[0] = mxCreateNumericMatrix(m,m,classid,cplxflag);
        if (cplx) {
            Lpr = mxGetData(plhs[0]);
            Lpi = mxGetImagData(plhs[0]);
            for (j=0; j<m; j++) {
                start = j<m ? j:m;
                for (i=start; i<m; i++) {
                    Lpr[j*m+i] = Ap[j*2*lda+2*i];
                    Lpi[j*m+i] = Ap[j*2*lda+2*i+1];
                }
            }
        }
        else {
            Lpr = mxGetData(plhs[0]);
            for (j=0; j<m; j++) {
                start = j<m ? j:m;
                for (i=start; i<m; i++) {
                    Lpr[j*m+i] = Ap[j*lda+i];
                }
            }
        }
    }
    else {
        plhs[0] = mxCreateNumericMatrix(m,n,classid,cplxflag);
        if (cplx) {
            Lpr = mxGetData(plhs[0]);
            Lpi = mxGetImagData(plhs[0]);
            for (j=0; j<n; j++) {
                start = j<min_mn ? j:min_mn;
                for (i=start; i<m; i++) {
                    Lpr[j*m+i] = Ap[j*2*lda+2*i];
                    Lpi[j*m+i] = Ap[j*2*lda+2*i+1];
                }
            }
        }
        else {
            Lpr = mxGetData(plhs[0]);
            for (j=0; j<n; j++) {
                start = j<min_mn ? j:min_mn;
                for (i=start; i<m; i++) {
                    Lpr[j*m+i] = Ap[j*lda+i];
                }
            }
        }
    }

    if (nlhs == 2) {
        m2 = (econ == 1) ? min_mn : n;

        /* determine blocksize */
        lwork = -1;
        psize2 = mxMalloc(dc*element_size);
        if (cplx) {
            cunglq(&m2, &n, &min_mn, Ap, &lda, ptau, psize2, &lwork, &info);
        }
        else {
            sorglq(&m2, &n, &min_mn, Ap, &lda, ptau, psize2, &lwork, &info);
        }
        lwork = psize2[0];
        mxFree(psize2);

        /* allocate workspace */
        pwork2 = mxMalloc(dc*lwork*element_size);

        /* calls the SORGLQ function */
        if (cplx) {
            cunglq(&m2, &n, &min_mn, Ap, &lda, ptau, pwork2, &lwork, &info);
        }
        else {
            sorglq(&m2, &n, &min_mn, Ap, &lda, ptau, pwork2, &lwork, &info);
        }
        mxFree(pwork2);
        if (info != 0) {
            mxFree(ptau);
            mxFree(Ap);
            if (cplx) {
                mexErrMsgTxt("CUNGLQ not successful");
            }
            else {
                mexErrMsgTxt("SORGLQ not successful");
            }
        }

        /* allocate Q matrix */
        plhs[1] = mxCreateNumericMatrix(m2,n,classid,cplxflag);
        if (cplx) {
            Qpr = mxGetData(plhs[1]);
            Qpi = mxGetImagData(plhs[1]);

            /* copy Ap to Qp */
            for (i=0; i<m2; i++) {
                for (j=0; j<n; j++) {
                    Qpr[j*m2+i] = Ap[j*2*lda+2*i];
                    Qpi[j*m2+i] = Ap[j*2*lda+2*i+1];
                }
            }
        }
        else {
            Qpr = mxGetData(plhs[1]);

            /* copy Ap to Qp */
            for (i=0; i<m2; i++) {
                for (j=0; j<n; j++) {
                    Qpr[j*m2+i] = Ap[j*lda+i];
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
    if (nrhs != 1 && nrhs != 2) {
        mexErrMsgTxt("LQ requires one or two input arguments.");
    }
    if (nlhs > 2) {
        mexErrMsgTxt("Too many output arguments.");
    }
    if (!mxIsNumeric(prhs[0]) || mxIsSparse(prhs[0])) {
        mexErrMsgTxt( "Input must be a full matrix." );
    }
    if (mxIsDouble(prhs[0])) {
        lq_double(nlhs, plhs, nrhs, prhs);
    }
    else if (mxIsSingle(prhs[0])) {
        lq_single(nlhs, plhs, nrhs, prhs);
    }
    else {
        mexErrMsgTxt( "Class is not supported." );
    }
}

