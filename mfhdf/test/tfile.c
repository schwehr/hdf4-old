/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF.  The full HDF copyright notice, including       *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at      *
 * http://hdfgroup.org/products/hdf4/doc/Copyright.html.  If you do not have *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef RCSID
static char RcsId[] = "@(#)$Revision: 5218 $";
#endif

/* $Id: tfile.c 5218 2009-08-29 04:21:49Z bmribler $ */

#include "mfhdf.h"

#ifdef HDF

#include "hdftest.h"

/********************************************************************
   Name: test_file_inuse() - tests preventing of an in-use file being 
			     removed at cleanup time.

   Description: 
	Sometime, when an error occurs, the cleanup process attempts to 
	remove a file, which might still be in use (part of bugzilla #376.)  
	The routine test_file_inuse is to test the fix that provides the 
	underlaying call to HPisfile_in_use, which should successfully 
	determines whether a file is still in use before an attempt to remove.

	The main contents include:
	- a loop that repeatedly calls SDstart/DFACC_CREATE; only the first
	  SDstart succeeds, the subsequent ones should fail.
	- SDcreate, SDwritedata, SDendaccess follow
	- outside of that loop is another loop to call SDend corresponding
	  to the previous SDstart's
	- then, at the end, the file will be reopened; if the file doesn't
	  exist and causes SDstart to fail, the test will fail.

	Before the fix, when the 2nd SDstart/DFACC_CREATE was called and
	failed because the file was being in use from the first call to
	SDstart/DFACC_CREATE, the cleaning process removed the file.

   Return value:
	The number of errors occurred in this routine.

   BMR - Jun 22, 2005
*********************************************************************/

#define FILE_NAME     "bug376.hdf"	/* data file to test */
#define DIM0 10

static intn
test_file_inuse()
{
    int32 file_id, sd_id[5], sds_id[5];
    intn statusn;
    int32 dims[1], start[1], edges[1], rank;
    int16 array_data[DIM0];
    char* names[5] = {"data1", "data2", "data3", "data4", "data5"};
    intn i, j;
    intn      num_errs = 0;     /* number of errors so far */

    for (i=0; i<5; i++)
    {
        /* Create and open the file and initiate the SD interface. */
        sd_id[i] = SDstart(FILE_NAME, DFACC_CREATE);
	if (i == 0) {
	    CHECK(sd_id[i], FAIL, "SDstart"); } /* 1st SDstart must pass */
	else {
	    VERIFY(sd_id[i], FAIL, "SDstart"); }
	/* subsequent SDstart should fail, which causes the following calls
	   to fail as well */

        /* Define the rank and dimensions of the data sets to be created. */
        rank = 1;
        dims[0] = DIM0;
        start[0] = 0;
        edges[0] = DIM0;

        /* Create the array data set. */
        sds_id[i] = SDcreate(sd_id[i], names[i], DFNT_INT16, rank, dims);
	if (i == 0) {
	    CHECK(sds_id[i], FAIL, "SDcreate"); } /* 1st SDcreate must pass */
	else
	    VERIFY(sds_id[i], FAIL, "SDcreate");

        /* Fill the stored-data array with values. */
        for (j = 0; j < DIM0; j++) {
                        array_data[j] = (i + 1)*(j + 1);
        }

	/* Write data to the data set */
	statusn = SDwritedata(sds_id[i], start, NULL, edges, (VOIDP)array_data);
	if (i == 0) {
	    CHECK(statusn, FAIL, "SDwritedata"); } /* 1st SDwritedata must pass */
	else
	    VERIFY(statusn, FAIL, "SDwritedata");

        /* Terminate access to the data sets. */
        statusn = SDendaccess(sds_id[i]);
	if (i == 0) {
	    CHECK(statusn, FAIL, "SDendaccess"); } /* 1st SDendaccess must pass */
	else
	    VERIFY(statusn, FAIL, "SDendaccess");

    } /* for i */

    for (i=0; i<5; i++)
    {
        /* Terminate access to the SD interface and close the file. */
        statusn = SDend (sd_id[i]);
	if (i == 0) {
	    CHECK(statusn, FAIL, "SDend"); } /* 1st SDend must pass */
	else
	    VERIFY(statusn, FAIL, "SDend");
    }

    /* Try to open the file, which should exist */
    file_id = SDstart(FILE_NAME, DFACC_RDWR);
    CHECK(file_id, FAIL, "SDstart");

    statusn = SDend (file_id);
    CHECK(statusn, FAIL, "SDend");

    return num_errs;
}   /* test_file_inuse */

/********************************************************************
   Name: test_max_open_files() - tests the new API SDreset_maxopenfiles,
				SDget_maxopenfiles, SDget_numopenfiles,
				and SDgetfilename.

   Description: 
	There were multiple requests from the users to increase the maximum
	number of opened files allowed.  SDreset_maxopenfiles is added to
	allow the user to reset that value.  The current default value is 32.
	This API can be called anytime to increase it.  This test routine will 
	carry out the following tests:

	- Get the current max, should be the default (32,) and the system limit
	- Reset current max to an arbitrary number that is larger than the 
	  default and verify
	- Try to create more files than the current max and all should 
	  succeed, because NC_open resets the current max to system limit 
	  automatically, when the number of opened files exceeds the current
	  max
	- Get the current max and system limit and verify, current max 
	  should be the system limit
	- Get the current max another way, it should be the system limit again
	- Get the current number of files being opened
	- Reset current max to a value that is smaller than the current
	  number of opened files; it shouldn't reset
	- Reset current max again to a value that is smaller than the
	  current max but larger than the current number of opened files,
	  that should work for there is no information loss
	- Try to create more files up to the system limit or NUM_FILES_HI,
	  because the arrays have max NUM_FILES_HI elements in this test
	- Close all the files, then try opening all again to verify their 
	  names, this is to test bugzilla 440

   Return value:
	The number of errors occurred in this routine.

   BMR - Oct 14, 2005
*********************************************************************/

#define NUM_FILES_LOW 35
#define NUM_FILES_HI  1024

static int test_max_open_files()
{
    int32 fids[NUM_FILES_HI];		/* holds IDs of opened files */
    char  filename[NUM_FILES_HI][10];	/* holds generated file names */
    char  readfname[H4_MAX_NC_NAME];	/* file name retrieved from file id */
    intn  index, status,
	  curr_max, 	/* curr maximum number of open files allowed in HDF */
	  sys_limit, 	/* maximum number of open files allowed by system */
	  curr_max_bk,  /* back up of curr_max */
	  curr_opened,	/* number of files currently being opened */
	  temp_limit,	/* temp var - num of files to be opened in this test */
	  num_errs = 0;	/* number of errors so far */

    /* Get the current max and system limit */
    status = SDget_maxopenfiles(&curr_max, &sys_limit);
    CHECK(status, FAIL, "test_maxopenfiles: SDget_maxopenfiles");
    VERIFY(curr_max, H4_MAX_NC_OPEN, "test_maxopenfiles: SDreset_maxopenfiles");

    /* Reset current max to an arbitrary number and check */
    curr_max = SDreset_maxopenfiles(33);
    CHECK(status, FAIL, "test_maxopenfiles: SDreset_maxopenfiles");
    VERIFY(curr_max, 33, "test_maxopenfiles: SDreset_maxopenfiles");

    /* Try to create more files than the default max (currently, 32) and
       all should succeed */
    for (index=0; index < NUM_FILES_LOW; index++)
    {
	/* Create a file */
	sprintf(filename[index], "file%i", index);
	fids[index] = SDstart(filename[index], DFACC_CREATE);
	CHECK(fids[index], FAIL, "test_maxopenfiles: SDstart");
    }

    /* Verify that NUM_FILES_LOW files are opened */
    curr_opened = SDget_numopenfiles();
    VERIFY(curr_opened, NUM_FILES_LOW, "test_maxopenfiles: SDget_numopenfiles");

    /* Now randomly close 3 files and check number of opened files */
    status = SDend(fids[5]);
    CHECK(status, FAIL, "test_maxopenfiles: SDend");
    status = SDend(fids[15]);
    CHECK(status, FAIL, "test_maxopenfiles: SDend");
    status = SDend(fids[25]);
    CHECK(status, FAIL, "test_maxopenfiles: SDend");
    curr_opened = SDget_numopenfiles();
    VERIFY(curr_opened, NUM_FILES_LOW-3, "test_maxopenfiles: SDget_numopenfiles");

    /* Get the current max and system limit */
    status = SDget_maxopenfiles(&curr_max, &sys_limit);
    CHECK(status, FAIL, "test_maxopenfiles: SDget_maxopenfiles");
    VERIFY(curr_max, sys_limit, "test_maxopenfiles: SDreset_maxopenfiles");

    /* Get the current max another way, it should be the system limit */
    curr_max = SDreset_maxopenfiles(0);
    VERIFY(curr_max, sys_limit, "test_maxopenfiles: SDreset_maxopenfiles");

    /* Reopen the 3 files above, and check the number of opened files again */
    fids[5] = SDstart(filename[5], DFACC_RDWR);
    CHECK(fids[5], FAIL, "test_maxopenfiles: SDstart");
    fids[15] = SDstart(filename[15], DFACC_RDWR);
    CHECK(fids[15], FAIL, "test_maxopenfiles: SDstart");
    fids[25] = SDstart(filename[25], DFACC_RDWR);
    CHECK(fids[25], FAIL, "test_maxopenfiles: SDstart");
    curr_opened = SDget_numopenfiles();
    VERIFY(curr_opened, NUM_FILES_LOW, "test_maxopenfiles: SDget_numopenfiles");

    /* Reset current max to a value that is smaller than the current 
       number of opened files; it shouldn't reset */
    curr_max_bk = curr_max;
    curr_max = SDreset_maxopenfiles(curr_opened-1);
    VERIFY(curr_max, curr_max_bk, "test_maxopenfiles: SDreset_maxopenfiles");

    /* Reset current max again to a value that is smaller than the 
       current max but larger than the current number of opened files, 
       that should work for there is no information loss */
    curr_max = SDreset_maxopenfiles(curr_opened+3);
    VERIFY(curr_max, curr_opened+3, "test_maxopenfiles: SDreset_maxopenfiles");

    /* Try to create more files up to the system limit or NUM_FILES_HI,
       because the arrays have max NUM_FILES_HI elements in this test */
    temp_limit = sys_limit / 2;
    temp_limit = temp_limit > NUM_FILES_HI ? NUM_FILES_HI : temp_limit;
    for (index=NUM_FILES_LOW; index < temp_limit; index++)
    {
        /* Create a file */
	sprintf(filename[index], "file%i", index);
	fids[index] = SDstart(filename[index], DFACC_CREATE);

        /* if SDstart fails due to "too many open files," then adjust
           temp_limit so that further failure can be prevented, i.e.
           following SDend and SDstart */
        if ((fids[index] == FAIL) && (HEvalue(1) == DFE_TOOMANY))
            temp_limit = index;

        /* only CHECK returned value from SDstart if the failure wasn't
           because of "too many open files" -BMR 2006/11/01 */
        else
            CHECK(fids[index], FAIL, "test_maxopenfiles: SDstart");
    }

    /* Close all the files, then try opening all again to verify their 
       names, this is to test bugzilla 440 */
    for (index=0; index < temp_limit; index++)
    {
	status = SDend(fids[index]);
	CHECK(status, FAIL, "test_maxopenfiles: SDend");

	fids[index] = SDstart(filename[index], DFACC_RDWR);
	CHECK(fids[index], FAIL, "test_maxopenfiles: SDstart");
    }

    /* Verify their names */
    for (index=0; index < temp_limit; index++)
    {
	status = SDgetfilename(fids[index], readfname);
	CHECK(status, FAIL, "test_maxopenfiles: SDgetfilename");

	/* Verify the file name retrieved against the original */
	if (HDstrcmp(readfname, filename[index]))
	{
	    fprintf(stderr, "SDgetfilename: incorrect file being opened - expected <%s>, retrieved <%s>\n", filename[index], readfname);
	}
    }

    /* Close then remove all the files */
    for (index=0; index < temp_limit; index++)
    {
	status = SDend(fids[index]);
	CHECK(status, FAIL, "test_maxopenfiles: SDend");
	remove(filename[index]);
    }
    return num_errs;
}

/********************************************************************
   Name: test_longfilename() - tests that the library can handle a very
				long file name (bugzilla 1331.)

   Description: 
	The main contents include:
	- create a file with very long name
	- create a dataset and close it (this is to activate the failure)
	- SDend to close the file, and it would cause segfault before the
	  fix was applied.

   Return value:
	The number of errors occurred in this routine.

   BMR - Jan 16, 2009
*********************************************************************/

#define NX 2
#define NY 3
static int
test_longfilename()
{
    int32 fid;                  /* file id */
    int32 dset1;                /* dataset ids */
    int32 dims[2];              /* variable shapes */
    int   ii;
    char dsname[10];
    char filename[256];
    intn  status = 0;           /* status returned by called functions */
    intn  num_errs = 0;         /* number of errors so far */

    strcpy(dsname, "dataset 1");
    strcpy(filename, "This file name has quite a few characters because it is used to test the fix of bugzilla 1331. It has to be at least this long to see.");

    /* enter define mode */
    fid = SDstart(filename, DFACC_CREATE);
    CHECK(fid, FAIL, "test_longfilename: SDstart");

    /* Define dimension of the data set to be created. */
    dims[0] = NX;
    dims[1] = NY;

    /* Create a dataset to reproduce the problem before the fix */
    dset1 = SDcreate(fid, dsname, DFNT_FLOAT32, 2, dims);
    CHECK(dset1, FAIL, "test_longfilename: SDcreate");

    status = SDendaccess(dset1);
    CHECK(status, FAIL, "test_longfilename: SDendaccess");

    status = SDend(fid);
    CHECK(fid, FAIL, "test_longfilename: SDend");

    return num_errs;
}


/* Test driver for testing miscellaneous file related APIs. */
extern int
test_files()
{
    intn num_errs = 0;         /* number of errors */

    /* Output message about test being performed */
    TESTING("miscellaneous file related functions (tfile.c)");

    /* test that an in-use file is not removed in certain failure 
       cleanup. 06/21/05 - bugzilla 376 - BMR */
    num_errs = num_errs + test_file_inuse();

    /* test APIs that were added for fixing bugzilla 396 and 440.  
       09/07/05 - BMR */
    num_errs = num_errs + test_max_open_files();

    /* test the fix of bugzzila 1331. 01/16/09 - BMR */
    num_errs = num_errs + test_longfilename();

    if (num_errs == 0) PASSED();
    return num_errs;
}

#endif /* HDF */

