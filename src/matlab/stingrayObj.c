/* ==========================================================================
 * stingrayObj.c 
 * 
 *==========================================================================*/

#include "mex.h"
#include "string.h"
#include <stdio.h>

#define MAXCHARS 80   /* max length of string contained in each field */

/*  the gateway routine.  */
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
    const char **fnames;       /* pointers to field names */
    const mwSize *dims;
    mxArray    *tmp, *fout;
    char       *pdata=NULL;
    int        ifield;
    mxClassID  *classIDflags;
    mwIndex    jstruct;
    mwSize     NSplitElems;
    mwSize     NSharedElems;
    mwSize     ndim;
    
    /* check proper input and output */
    if(nrhs!=2)
        mexErrMsgIdAndTxt( "MATLAB:stingrayObj:invalidNumInputs",
                "Two inputs required.");
    else if(nlhs > 1)
        mexErrMsgIdAndTxt( "MATLAB:stingrayObj:maxlhs",
                "Too many output arguments.");
    else if(!mxIsCell(prhs[0]))
        mexErrMsgIdAndTxt( "MATLAB:stingrayObj:inputNotCell",
                "Input must be a cell.");
    
    /* get input arguments */
    NSplitElems = mxGetNumberOfElements(prhs[0]);
    NSharedElems = mxGetNumberOfElements(prhs[1]);
    printf("NSplitElems : %d\n", NSplitElems);
    printf("NSharedElems : %d\n", NSharedElems);

    /* Assure that the datatypes are consistent with Stingray */

   /*for(jstruct = 0; jstruct < NStructElems; jstruct++) {
    *    tmp = mxGetFieldByNumber(prhs[0], jstruct, ifield);
    *    if(tmp == NULL) {
    *        mexPrintf("%s%d\t%s%d\n", "FIELD: ", ifield+1, "STRUCT INDEX :", jstruct+1);
    *        mexErrMsgIdAndTxt( "MATLAB:phonebook:fieldEmpty",
    *                "Above field is empty!");
    *    }
    *    if(jstruct==0) {
    *        if( (!mxIsChar(tmp) && !mxIsNumeric(tmp)) || mxIsSparse(tmp)) {
    *            mexPrintf("%s%d\t%s%d\n", "FIELD: ", ifield+1, "STRUCT INDEX :", jstruct+1);
    *            mexErrMsgIdAndTxt( "MATLAB:phonebook:invalidField",
    *                    "Above field must have either string or numeric non-sparse data.");
    *        }
    *        classIDflags[ifield]=mxGetClassID(tmp);
    *    } else {
    *        if (mxGetClassID(tmp) != classIDflags[ifield]) {
    *            mexPrintf("%s%d\t%s%d\n", "FIELD: ", ifield+1, "STRUCT INDEX :", jstruct+1);
    *            mexErrMsgIdAndTxt( "MATLAB:phonebook:invalidFieldType",
    *                    "Inconsistent data type in above field!");
    *        } else if(!mxIsChar(tmp) &&
    *                ((mxIsComplex(tmp) || mxGetNumberOfElements(tmp)!=1))){
    *            mexPrintf("%s%d\t%s%d\n", "FIELD: ", ifield+1, "STRUCT INDEX :", jstruct+1);
    *            mexErrMsgIdAndTxt( "MATLAB:phonebook:fieldNotRealScalar",
    *                    "Numeric data in above field must be scalar and noncomplex!");
    *        }
    *    }
    *}
   */


    /* allocate memory  for storing pointers 
    fnames = mxCalloc(nfields, sizeof(*fnames));*/
    /* get field name pointers 
    for (ifield=0; ifield< nfields; ifield++){
        fnames[ifield] = mxGetFieldNameByNumber(prhs[0],ifield);
    }*/

    return;
}

