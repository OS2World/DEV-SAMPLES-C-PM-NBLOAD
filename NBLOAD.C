/*********************************************************************
 *                                                                   *
 * MODULE NAME :  nbload.c               AUTHOR:  Rick Fishman       *
 * DATE WRITTEN:  10-18-92                                           *
 *                                                                   *
 * HOW TO RUN THIS PROGRAM:                                          *
 *                                                                   *
 *  Just enter NBLOAD on the command line.                           *
 *                                                                   *
 *   Arguments:                                                      *
 *                                                                   *
 *       0 - Load dialogs on demand                                  *
 *       1 - Use a timer                                             *
 *       2 - Load them at startup                                    *
 *                                                                   *
 *                                                                   *
 * MODULE DESCRIPTION:                                               *
 *                                                                   *
 *  Only module for NBLOAD.EXE, a program that builds on the source  *
 *  code of NBBASE.EXE. It attempts to demonstrate the perceived     *
 *  load time of NBBASE.EXE using 3 methods of loading the dialog    *
 *  boxes that will be associated with notebook pages:               *
 *                                                                   *
 *    1. Delay loading them until they are needed.                   *
 *    2. Set a timer that on a timer interval goes thru the list of  *
 *       dialogs that need to be loaded. If it finds one that hasn't *
 *       yet been loaded, load it. This technique also uses number 2 *
 *       because the dialog will be loaded if the user requests it   *
 *       and it hasn't been loaded and the timer hasn't yet loaded   *
 *       it.                                                         *
 *    3. Load them all at startup of the Notebook.                   *
 *                                                                   *
 * NOTES:                                                            *
 *                                                                   *
 *  This program is strictly a sample and should be treated as such. *
 *  There is nothing real-world about it and the dialogs that it     *
 *  uses do nothing useful. It is meant solely to see the differences*
 *  of loading Notebook dialogs using various techniques.            *
 *                                                                   *
 *  I hope this code proves useful for other PM programmers. The     *
 *  more of us the better!                                           *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-18-92 - Program coded                                         *
 *  12-04-92 - Brought up to spec for changes in NBBASE              *
 *                                                                   *
 *  Rick Fishman                                                     *
 *  Code Blazers, Inc.                                               *
 *  4113 Apricot                                                     *
 *  Irvine, CA. 92720                                                *
 *  CIS ID: 72251,750                                                *
 *                                                                   *
 *********************************************************************/

#pragma strings(readonly)   // used for debug version of memory mgmt routines

/*********************************************************************/
/*------- Include relevant sections of the OS/2 header files --------*/
/*********************************************************************/

#define  INCL_GPILCIDS
#define  INCL_GPIPRIMITIVES
#define  INCL_WINDIALOGS
#define  INCL_WINERRORS
#define  INCL_WINFRAMEMGR
#define  INCL_WINMESSAGEMGR
#define  INCL_WINSTDBOOK
#define  INCL_WINSYS
#define  INCL_WINTIMER
#define  INCL_WINWINDOWMGR

/**********************************************************************/
/*----------------------------- INCLUDES -----------------------------*/
/**********************************************************************/

#include <os2.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "nbload.h"

/*********************************************************************/
/*------------------- APPLICATION DEFINITIONS -----------------------*/
/*********************************************************************/

#define USAGE_MSG             "Usage: NBLOAD [0 1 2]\n\n"                    \
                              "0 - Load dialogs on demand (default)\n"       \
                              "1 - Load dialogs on a timer\n"                \
                              "2 - Load all dialogs at startup"

#define FRAME_FLAGS           (FCF_TASKLIST | FCF_TITLEBAR   | FCF_SYSMENU | \
                               FCF_MINMAX   | FCF_SIZEBORDER | FCF_ICON)

#define TAB_WIDTH_MARGIN      10   // Padding for the width of a notebook tab
#define TAB_HEIGHT_MARGIN     6    // Padding for the height of a notebook tab
#define DEFAULT_NB_TAB_HEIGHT 16   // Default if Gpi calls fail

#define FRAME_X               10   // In dialog units!
#define FRAME_Y               10   // In dialog units!
#define FRAME_CX              275  // In dialog units!
#define FRAME_CY              210  // In dialog units!

#define LOAD_ON_DEMAND        0    // Constants for commandline value
#define LOAD_BY_TIMER         1
#define LOAD_AT_STARTUP       2
#define LOAD_MAX_VALUE        LOAD_AT_STARTUP

#define TIMER_INTERVAL        1000 // 1 second timer interval if LOAD_BY_TIMER

/**********************************************************************/
/*----------------------- FUNCTION PROTOTYPES ------------------------*/
/**********************************************************************/

       INT  main             ( INT argc, CHAR **argv );
static BOOL Init             ( INT argc, CHAR **argv );
static BOOL TurnToFirstPage  ( HWND hwndClient );
static BOOL SetFramePos      ( HWND hwndFrame );
static BOOL CreateNotebook   ( HWND hwndClient );
static BOOL SetUpPage        ( HWND hwndNB, INT iArrayIndex );
static BOOL SetTabDimensions ( HWND hwndNB );
static INT  GetStringSize    ( HPS hps, HWND hwndNB, PSZ szString);
static BOOL ControlMsg       ( USHORT usCtl, USHORT usEvent, MPARAM mp2);
static VOID SetNBPage        ( PPAGESELECTNOTIFY ppsn );
static VOID CheckDialogs     ( HWND hwndClient );
static HWND LoadAndAssociate ( HWND hwndNB, ULONG ulPageId, PNBPAGE pnbp );
static VOID Msg              ( PSZ szFormat, ... );

FNWP wpClient, wpPage;

/**********************************************************************/
/*------------------------ GLOBAL VARIABLES --------------------------*/
/**********************************************************************/

INT iLoadType;       // Way to load dialogs - can be modified by cmdline parm

NBPAGE nbpage[] =    // INFORMATION ABOUT NOTEBOOK PAGES (see NBLOAD.H)
{
    { wpPage,      "Page 1",  "Page ~1",  IDD_PAGE1,  EF_1,  FALSE, BKA_MAJOR },
    { (PFNWP) NULL,"Page 2",  "Page ~2",  0,          0,     TRUE,  BKA_MAJOR },
    { wpPage,      "Page 2A", "Page 2~A", IDD_PAGE2A, EF_2A, FALSE, BKA_MINOR },
    { wpPage,      "Page 2B", "Page 2~B", IDD_PAGE2B, EF_2B, FALSE, BKA_MINOR },
    { wpPage,      "Page 3",  "Page ~3",  IDD_PAGE3,  EF_3,  FALSE, BKA_MAJOR },
    { (PFNWP) NULL,"Page 4",  "Page ~4",  0,          0,     TRUE,  BKA_MAJOR },
    { wpPage,      "Page 4A", "Page ~4A", IDD_PAGE4A, EF_4A, FALSE, BKA_MINOR },
    { wpPage,      "Page 4B", "Page ~4B", IDD_PAGE4B, EF_4B, FALSE, BKA_MINOR },
    { wpPage,      "Page 4C", "Page ~4C", IDD_PAGE4C, EF_4C, FALSE, BKA_MINOR },
    { wpPage,      "Page 4D", "Page ~4D", IDD_PAGE4D, EF_4D, FALSE, BKA_MINOR },
    { wpPage,      "Page 5",  "Page ~5",  IDD_PAGE5,  EF_5,  FALSE, BKA_MAJOR },
    { (PFNWP) NULL,"Page 6",  "Page ~6",  0,          0,     TRUE,  BKA_MAJOR },
    { wpPage,      "Page 6A", "Page ~6A", IDD_PAGE6A, EF_6A, FALSE, BKA_MINOR },
    { wpPage,      "Page 6B", "Page ~6B", IDD_PAGE6B, EF_6B, FALSE, BKA_MINOR },
    { wpPage,      "Page 6C", "Page ~6C", IDD_PAGE6C, EF_6C, FALSE, BKA_MINOR },

    { wpPage,"Page 7 (1 of 4)","Page ~7", IDD_PAGE71, EF_71, FALSE, BKA_MAJOR },
    { wpPage,"Page 7 (2 of 4)",NULL,      IDD_PAGE72, EF_72, FALSE, 0 },
    { wpPage,"Page 7 (3 of 4)",NULL,      IDD_PAGE73, EF_73, FALSE, 0 },
    { wpPage,"Page 7 (4 of 4)",NULL,      IDD_PAGE74, EF_74, FALSE, 0 }
};

#define PAGE_COUNT (sizeof( nbpage ) / sizeof( NBPAGE ))

/**********************************************************************/
/*------------------------------- main -------------------------------*/
/*                                                                    */
/*  PROGRAM ENTRYPOINT                                                */
/*                                                                    */
/*  INPUT: commandline                                                */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
INT main( INT argc, CHAR **argv )
{
    BOOL  fSuccess;
    HAB   hab;
    HMQ   hmq;
    HWND  hwndFrame, hwndClient;
    QMSG  qmsg;
    ULONG flFrame = FRAME_FLAGS;

    // This macro is defined for the debug version of the C Set/2 Memory
    // Management routines. Since the debug version writes to stderr, we
    // send all stderr output to a debuginfo file. Look in MAKEFILE to see how
    // to enable the debug version of those routines.

#ifdef __DEBUG_ALLOC__
    freopen( DEBUG_FILENAME, "w", stderr );
#endif

    hab = WinInitialize( 0 );

    if( hab )
        hmq = WinCreateMsgQueue( hab, 0 );
    else
    {
        WinAlarm( HWND_DESKTOP, WA_ERROR );

        (void) fprintf( stderr, "WinInitialize failed!" );
    }

    if( hmq )
    {
        // CS_CLIPCHILDREN so the client doesn't need to paint the area covered
        // by the notebook. CS_SIZEREDRAW so the notebook gets sized correctly
        // the first time the Frame/Client get drawn.

        fSuccess = WinRegisterClass( hab, NOTEBOOK_WINCLASS, wpClient,
                                     CS_CLIPCHILDREN | CS_SIZEREDRAW, 0 );
        if( !fSuccess )
            Msg( "WinRegisterClass RC(%X)", HABERR( hab ) );
    }
    else
    {
        WinAlarm( HWND_DESKTOP, WA_ERROR );

        (void) fprintf( stderr, "WinCreateMsgQueue RC(%X)", HABERR( hab ) );
    }

    if( fSuccess && Init( argc, argv ) )
        hwndFrame = WinCreateStdWindow( HWND_DESKTOP, 0, &flFrame,
                                        NOTEBOOK_WINCLASS, NULL, 0, NULLHANDLE,
                                        ID_NBWINFRAME, &hwndClient );

    if( hwndFrame )
    {
        // If the TURNTOPAGE is sent during WM_CREATE processing, the dialog
        // box for page 1 won't be visible.

        fSuccess = TurnToFirstPage( hwndClient );

        if( fSuccess )
            fSuccess = SetFramePos( hwndFrame );

        if( fSuccess )
            WinSetWindowText( hwndFrame, PROGRAM_TITLE );
    }

    if( hwndFrame )
    {
        while( WinGetMsg( hab, &qmsg, NULLHANDLE, 0, 0 ) )
            WinDispatchMsg( hab, &qmsg );

        WinDestroyWindow( hwndFrame );
    }

    if( hmq )
        WinDestroyMsgQueue( hmq );

    if( hab )
        WinTerminate( hab );

#ifdef __DEBUG_ALLOC__
    _dump_allocated( -1 );
#endif

    return 0;
}

/**********************************************************************/
/*-------------------------------- Init ------------------------------*/
/*                                                                    */
/*  PERFORM PROGRAM INITIALIZATION                                    */
/*                                                                    */
/*  INPUT: commandline                                                */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL Init( INT argc, CHAR **argv )
{
    BOOL fSuccess = TRUE;

    if( argc > 1 )
        iLoadType = atoi( argv[ 1 ] );

    if( iLoadType > LOAD_MAX_VALUE )
    {
        fSuccess = FALSE;

        Msg( USAGE_MSG );
    }

    return fSuccess;
}

/**********************************************************************/
/*----------------------------- wpClient -----------------------------*/
/*                                                                    */
/*  CLIENT WINDOW PROCEDURE                                           */
/*                                                                    */
/*  INPUT: window proc params                                         */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: retcode from processing message                           */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
MRESULT EXPENTRY wpClient( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    switch( msg )
    {
        case WM_CREATE:

            // Don't create the window if notebook creation fails

            if( !CreateNotebook( hwnd ) )
                return (MRESULT) TRUE;

            break;


        case WM_SIZE:

            // Size the notebook with the client window

            WinSetWindowPos( WinWindowFromID( hwnd, ID_NB ), 0, 0, 0,
                             SHORT1FROMMP( mp2 ), SHORT2FROMMP( mp2 ),
                             SWP_SIZE | SWP_SHOW );

            break;


        case WM_ERASEBACKGROUND:

            // Paint the client in the default background color

            return (MRESULT) TRUE;


        case WM_CONTROL:

            if( ControlMsg( SHORT1FROMMP( mp1 ), SHORT2FROMMP( mp1 ), mp2 ) )
                return 0;
            else
                break;


        case WM_TIMER:

            // See if any dialogs need to be loaded

            CheckDialogs( hwnd );

            break;


        case WM_DESTROY:

            if( iLoadType == LOAD_BY_TIMER )
                WinStopTimer( ANCHOR( hwnd ), hwnd, 1 );

            break;
    }

    return WinDefWindowProc( hwnd, msg, mp1, mp2 );
}

/**********************************************************************/
/*------------------------- TurnToFirstPage --------------------------*/
/*                                                                    */
/*  TURN TO THE FIRST PAGE IN THE NOTEBOOK.                           */
/*                                                                    */
/*  INPUT: client window handle                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL TurnToFirstPage( HWND hwndClient )
{
    HWND  hwndNB = WinWindowFromID( hwndClient, ID_NB );
    ULONG ulFirstPage;
    BOOL  fSuccess = TRUE;

    ulFirstPage = (ULONG) WinSendMsg( hwndNB, BKM_QUERYPAGEID, NULL,
                                      MPFROM2SHORT( BKA_FIRST, BKA_MAJOR ) );

    if( ulFirstPage )
    {
        fSuccess = (ULONG) WinSendMsg( hwndNB, BKM_TURNTOPAGE,
                                       MPFROMLONG( ulFirstPage ), NULL );

        if( !fSuccess )
            Msg( "TurnToFirstPage BKM_TURNTOPAGE RC(%X)", HWNDERR( hwndNB ) );
    }
    else
    {
        fSuccess = FALSE;

        Msg( "TurnToFirstPage BKM_QUERYPAGEID RC(%X)", HWNDERR( hwndNB ) );
    }

    return fSuccess;
}

/**********************************************************************/
/*---------------------------- SetFramePos ---------------------------*/
/*                                                                    */
/*  SET THE FRAME ORIGIN AND SIZE.                                    */
/*                                                                    */
/*  INPUT: frame window handle                                        */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL SetFramePos( HWND hwndFrame )
{
    BOOL   fSuccess;
    POINTL aptl[ 2 ];

    // Convert origin and size from dialog units to pixels. We need to do this
    // because dialog boxes are automatically sized to the display. Since the
    // notebook contains these dialogs it must size itself accordingly so the
    // dialogs fit in the notebook.

    aptl[ 0 ].x = FRAME_X;
    aptl[ 0 ].y = FRAME_Y;
    aptl[ 1 ].x = FRAME_CX;
    aptl[ 1 ].y = FRAME_CY;

    fSuccess = WinMapDlgPoints( HWND_DESKTOP, aptl, 2, TRUE );

    if( fSuccess )
    {
        fSuccess = WinSetWindowPos( hwndFrame, NULLHANDLE,
                               aptl[ 0 ].x, aptl[ 0 ].y, aptl[ 1 ].x, aptl[ 1 ].y,
                               SWP_SIZE | SWP_MOVE | SWP_SHOW | SWP_ACTIVATE );

        if( !fSuccess )
            Msg( "SetFramePos WinSetWindowPos RC(%X)", HWNDERR( hwndFrame ) );
    }
    else
        Msg( "WinMapDlgPoints RC(%X)", HWNDERR( hwndFrame ) );

    return fSuccess;
}

/**********************************************************************/
/*-------------------------- CreateNotebook --------------------------*/
/*                                                                    */
/*  CREATE THE NOTEBOOK WINDOW                                        */
/*                                                                    */
/*  INPUT: client window handle                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL CreateNotebook( HWND hwndClient )
{
    BOOL fSuccess = TRUE;
    HWND hwndNB;
    INT  i;

    // Create the notebook. Its parent and owner will be the client window.
    // Its pages will show on the bottom right of the notebook. Its major tabs
    // will be on the right and they will be rounded. The status text will be
    // centered. Its binding will be spiraled rather than solid. The tab text
    // will be left-justified.

    hwndNB = WinCreateWindow( hwndClient, WC_NOTEBOOK, NULL,
                BKS_BACKPAGESBR | BKS_MAJORTABRIGHT | BKS_ROUNDEDTABS |
                BKS_STATUSTEXTCENTER | BKS_SPIRALBIND | BKS_TABTEXTLEFT |
                WS_GROUP | WS_TABSTOP | WS_VISIBLE,
                0, 0, 0, 0, hwndClient, HWND_TOP, ID_NB, NULL, NULL );

    if( hwndNB )
    {
        // Set the page background color to grey so it is the same as a dlg box.

        if( !WinSendMsg( hwndNB, BKM_SETNOTEBOOKCOLORS,
                         MPFROMLONG( SYSCLR_FIELDBACKGROUND ),
                         MPFROMSHORT( BKA_BACKGROUNDPAGECOLORINDEX ) ) )
            Msg( "BKM_SETNOTEBOOKCOLORS failed! RC(%X)", HWNDERR( hwndClient ));

        if( !SetTabDimensions( hwndNB ) )
            fSuccess = FALSE;

        // Insert all the pages into the notebook and configure them. The dialog
        // boxes are not going to be loaded and associated with those pages yet.

        for( i = 0; i < PAGE_COUNT && fSuccess; i++ )
            fSuccess = SetUpPage( hwndNB, i );
    }
    else
    {
        fSuccess = FALSE;

        Msg( "Notebook creation failed! RC(%X)", HWNDERR( hwndClient ) );
    }

    if( fSuccess && iLoadType == LOAD_BY_TIMER )
    {
        fSuccess = WinStartTimer( ANCHOR( hwndClient ), hwndClient, 1,
                                  TIMER_INTERVAL );

        if( !fSuccess )
            Msg( "WinStartTimer failed! RC(%X)", HWNDERR( hwndClient ) );
    }

    return fSuccess;
}

/**********************************************************************/
/*----------------------------- SetUpPage ----------------------------*/
/*                                                                    */
/*  SET UP A NOTEBOOK PAGE.                                           */
/*                                                                    */
/*  INPUT: window handle of notebook control,                         */
/*         index into nbpage array                                    */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL SetUpPage( HWND hwndNB, INT iPage )
{
    BOOL  fSuccess = TRUE;
    ULONG ulPageId;

    // Insert a page into the notebook and store it in the array of page data.
    // Specify that it is to have status text and the window associated with
    // each page will be automatically sized by the notebook according to the
    // size of the page.

    ulPageId = (ULONG) WinSendMsg( hwndNB, BKM_INSERTPAGE, NULL,
                            MPFROM2SHORT( nbpage[ iPage ].usTabType |
                                          BKA_STATUSTEXTON | BKA_AUTOPAGESIZE,
                                          BKA_LAST ) );

    if( ulPageId )
    {
        // Insert a pointer to this page's info into the space available
        // in each page (its PAGE DATA that is available to the application).

        fSuccess = (BOOL) WinSendMsg( hwndNB, BKM_SETPAGEDATA,
                                      MPFROMLONG( ulPageId ),
                                      MPFROMP( &nbpage[ iPage ] ) );

        // Set the text into the status line.

        if( fSuccess )
        {
            fSuccess = (BOOL) WinSendMsg( hwndNB, BKM_SETSTATUSLINETEXT,
                                          MPFROMP( ulPageId ),
                        MPFROMP( nbpage[ iPage ].szStatusLineText ) );

            if( !fSuccess )
                Msg( "BKM_SETSTATUSLINETEXT RC(%X)", HWNDERR( hwndNB ) );
        }
        else
            Msg( "BKM_SETPAGEDATA RC(%X)", HWNDERR( hwndNB ) );

        // Set the text into the tab for this page.

        if( fSuccess )
        {
            if( nbpage[ iPage ].usTabType )
                fSuccess = (BOOL) WinSendMsg( hwndNB, BKM_SETTABTEXT,
                                         MPFROMP( ulPageId ),
                                         MPFROMP( nbpage[ iPage ].szTabText ) );

            if( fSuccess )
            {
                // If the user wanted us to load all dialogs at startup time,
                // do it.

                if( iLoadType == LOAD_AT_STARTUP && nbpage[iPage].idDlg )
                    if( !LoadAndAssociate( hwndNB, ulPageId, nbpage + iPage ) )
                        fSuccess = FALSE;
            }
            else
                Msg( "BKM_SETTABTEXT RC(%X)", HWNDERR( hwndNB ) );
        }
    }
    else
    {
        fSuccess = FALSE;

        Msg( "BKM_INSERTPAGE RC(%X)", HWNDERR( hwndNB ) );
    }

    return fSuccess;
}

/**********************************************************************/
/*-------------------------- SetTabDimensions ------------------------*/
/*                                                                    */
/*  SET THE DIMENSIONS OF THE NOTEBOOK TABS.                          */
/*                                                                    */
/*  INPUT: window handle of notebook control                          */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: TRUE or FALSE if successful or not                        */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL SetTabDimensions( HWND hwndNB )
{
    BOOL         fSuccess = TRUE;
    HPS          hps = WinGetPS( hwndNB );
    FONTMETRICS  fm;
    INT          i, iSize, iLongestMajText = 0, iLongestMinText = 0;

    if( !hps )
    {
        Msg( "SetTabDimensions WinGetPS RC(%X)", HWNDERR( hwndNB ) );

        return FALSE;
    }

    (void) memset( &fm, 0, sizeof( FONTMETRICS ) );

    // Calculate the height of a tab as the height of an average font character
    // plus a margin value.

    if( GpiQueryFontMetrics( hps, sizeof( FONTMETRICS ), &fm ) )
        fm.lMaxBaselineExt += (TAB_HEIGHT_MARGIN * 2);
    else
    {
        fm.lMaxBaselineExt = DEFAULT_NB_TAB_HEIGHT + (TAB_HEIGHT_MARGIN * 2);

        Msg( "SetTabDimensions GpiQueryFontMetrics RC(%X)", HWNDERR( hwndNB ) );
    }

    // Calculate the longest tab text for both the MAJOR and MINOR pages

    for( i = 0; i < PAGE_COUNT; i++ )
    {
        if( nbpage[ i ].usTabType )
        {
            iSize = GetStringSize( hps, hwndNB, nbpage[ i ].szTabText );

            if( nbpage[ i ].usTabType == BKA_MAJOR )
            {
                if( iSize > iLongestMajText )
                    iLongestMajText = iSize;
            }
            else
            {
                if( iSize > iLongestMinText )
                    iLongestMinText = iSize;
            }
        }
    }

    WinReleasePS( hps );

    // Add a margin amount to the longest tab text

    if( iLongestMajText )
        iLongestMajText += TAB_WIDTH_MARGIN;

    if( iLongestMinText )
        iLongestMinText += TAB_WIDTH_MARGIN;

    // Set the tab dimensions for the MAJOR and MINOR pages. Note that the
    // docs as of this writing say to use BKA_MAJOR and BKA_MINOR in mp2 but
    // you really need BKA_MAJORTAB and BKA_MINORTAB.

    if( iLongestMajText )
    {
        fSuccess = (BOOL) WinSendMsg( hwndNB, BKM_SETDIMENSIONS,
                    MPFROM2SHORT( iLongestMajText, (SHORT)fm.lMaxBaselineExt ),
                    MPFROMSHORT( BKA_MAJORTAB ) );

        if( !fSuccess )
            Msg( "BKM_SETDIMENSIONS(MAJOR) RC(%X)", HWNDERR( hwndNB ) );
    }

    if( fSuccess && iLongestMinText )
    {
        fSuccess = (BOOL) WinSendMsg( hwndNB, BKM_SETDIMENSIONS,
                    MPFROM2SHORT( iLongestMinText, (SHORT)fm.lMaxBaselineExt ),
                    MPFROMSHORT( BKA_MINORTAB ) );

        if( !fSuccess )
            Msg( "BKM_SETDIMENSIONS(MINOR) RC(%X)", HWNDERR( hwndNB ) );
    }

    return fSuccess;
}

/**********************************************************************/
/*-------------------------- GetStringSize ---------------------------*/
/*                                                                    */
/*  GET THE SIZE IN PIXELS OF A STRING.                               */
/*                                                                    */
/*  INPUT: presentation space handle,                                 */
/*         notebook window handle,                                    */
/*         pointer to string                                          */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static INT GetStringSize( HPS hps, HWND hwndNB, PSZ szString )
{
    POINTL aptl[ TXTBOX_COUNT ];

    // Get the size, in pixels, of the string passed.

    if( !GpiQueryTextBox( hps, strlen( szString ), szString, TXTBOX_COUNT,
                          aptl ) )
    {
        Msg( "GetStringSize GpiQueryTextBox RC(%X)", HWNDERR( hwndNB ) );

        return 0;
    }
    else
        return aptl[ TXTBOX_CONCAT ].x;
}

/**********************************************************************/
/*---------------------------- ControlMsg ----------------------------*/
/*                                                                    */
/*  THE ENTRY DIALOG PROC GOT A WM_CONTROL MESSAGE.                   */
/*                                                                    */
/*  INPUT: client window handle,                                      */
/*         control id,                                                */
/*         control event code,                                        */
/*         2nd message parameter from WM_CONTROL message              */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static BOOL ControlMsg( USHORT usControl, USHORT usEvent, MPARAM mp2 )
{
    BOOL fProcessed = FALSE;

    switch( usControl )
    {
        case ID_NB:

            switch( usEvent )
            {
                case BKN_PAGESELECTED:

                    // A new page has been selected by the user. If the dialog
                    // box needs to be loaded, load it and associate it with
                    // the new page.

                    SetNBPage( (PPAGESELECTNOTIFY) mp2 );

                    fProcessed = TRUE;

                    break;
            }

            break;
    }

    return fProcessed;
}

/**********************************************************************/
/*---------------------------- SetNBPage -----------------------------*/
/*                                                                    */
/*  SET THE TOP PAGE IN THE NOTEBOOK CONTROL.                         */
/*                                                                    */
/*  INPUT: pointer to the PAGESELECTNOTIFY struct                     */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID SetNBPage( PPAGESELECTNOTIFY ppsn )
{
    HWND hwndDlg;

    // Get a pointer to the page information that is associated with this page.
    // It was stored in the page's PAGE DATA in the SetUpPage function.

    PNBPAGE pnbp = (PNBPAGE) WinSendMsg( ppsn->hwndBook, BKM_QUERYPAGEDATA,
                                        MPFROMLONG( ppsn->ulPageIdNew ), NULL );

    if( !pnbp )
        return;
    else if( pnbp == (PNBPAGE) BOOKERR_INVALID_PARAMETERS )
    {
        Msg( "SetNBPage BKM_QUERYPAGEDATA Invalid page id" );

        return;
    }

    // If this is a BKA_MAJOR page and it is what this app terms a 'parent'
    // page, that means when the user selects this page we actually want to go
    // to its first MINOR page. So in effect the MAJOR page is just a dummy page
    // that has a tab that acts as a placeholder for its MINOR pages. If the
    // user is using the left arrow to scroll thru the pages and they hit this
    // dummy MAJOR page, that means they have already been to its MINOR pages in
    // reverse order. They would now expect to see the page before the dummy
    // MAJOR page, so we skip the dummy page. Otherwise the user is going the
    // other way and wants to see the first MINOR page associated with this
    // 'parent' page so we skip the dummy page and show its first MINOR page.

    if( pnbp->fParent )
    {
        ULONG ulPageFwd, ulPageNew;

        ulPageFwd = (ULONG) WinSendMsg( ppsn->hwndBook, BKM_QUERYPAGEID,
                                        MPFROMLONG( ppsn->ulPageIdNew ),
                                        MPFROM2SHORT( BKA_NEXT, BKA_MINOR ) );

        // If this is true, the user is going in reverse order

        if( ulPageFwd == ppsn->ulPageIdCur )
            ulPageNew = (ULONG) WinSendMsg( ppsn->hwndBook, BKM_QUERYPAGEID,
                                            MPFROMLONG( ppsn->ulPageIdNew ),
                                            MPFROM2SHORT(BKA_PREV, BKA_MAJOR) );
        else
            ulPageNew = ulPageFwd;

        if( ulPageNew == (ULONG) BOOKERR_INVALID_PARAMETERS )
            Msg( "SetNBPage BKM_QUERYPAGEID Invalid page specified" );
        else if( ulPageNew )
            if( !WinSendMsg( ppsn->hwndBook, BKM_TURNTOPAGE,
                             MPFROMLONG( ulPageNew ), NULL ) )
                Msg( "BKM_TURNTOPAGE RC(%X)", HWNDERR( ppsn->hwndBook ) );
    }
    else
    {
        hwndDlg = (HWND) WinSendMsg( ppsn->hwndBook, BKM_QUERYPAGEWINDOWHWND,
                                     MPFROMLONG( ppsn->ulPageIdNew ), NULL );

        if( hwndDlg == (HWND) BOOKERR_INVALID_PARAMETERS )
        {
            hwndDlg = NULLHANDLE;

            Msg( "SetNBPage BKM_QUERYPAGEWINDOWHWND Invalid page specified" );
        }
        else if( !hwndDlg )

            // It is time to load this dialog because the user has flipped pages
            // to a page that hasn't yet had the dialog associated with it.

            hwndDlg = LoadAndAssociate( ppsn->hwndBook, ppsn->ulPageIdNew,
                                        pnbp );
    }

    // Set focus to the first control in the dialog. This is not automatically
    // done by the notebook.

    if( !pnbp->fParent && hwndDlg )
        if( !WinSetFocus( HWND_DESKTOP,
                          WinWindowFromID( hwndDlg, pnbp->idFocus ) ) )
        {
            // Bug in 2.0! Developers left some debug code in there!

            USHORT usErr = HWNDERR( ppsn->hwndBook );

            if( usErr != PMERR_WIN_DEBUGMSG )
                Msg( "SetNBPage WinSetFocus RC(%X)", usErr );
        }

    return;
}

/**********************************************************************/
/*-------------------------- CheckDialogs ----------------------------*/
/*                                                                    */
/*  LOAD A DIALOG IF NOT ALL ARE LOADED YET.                          */
/*                                                                    */
/*  INPUT: client window handle                                       */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static VOID CheckDialogs( HWND hwndClient )
{
    HWND   hwndDlg, hwndNB = WinWindowFromID( hwndClient, ID_NB );
    ULONG  ulPageId = 1;
    USHORT usWhat = BKA_FIRST;

    // Enumerate thru all pages looking for the first one that hasn't yet
    // been associated with an hwnd.

    while( ulPageId )
    {
        ulPageId = (ULONG) WinSendMsg( hwndNB, BKM_QUERYPAGEID,
                               MPFROMLONG( ulPageId ),
                               MPFROM2SHORT( usWhat, 0 ) );

        if( ulPageId == (ULONG) BOOKERR_INVALID_PARAMETERS )
        {
            ulPageId = 0;

            Msg( "CheckDialogs BKM_QUERYPAGEID Invalid page specified" );

            break;
        }
        else if( ulPageId )
        {
            hwndDlg = (HWND) WinSendMsg( hwndNB, BKM_QUERYPAGEWINDOWHWND,
                                         MPFROMLONG( ulPageId ), NULL );

            if( hwndDlg == (HWND) BOOKERR_INVALID_PARAMETERS )
            {
                Msg( "CheckDialogs QUERYPAGEWNDHWND Invalid page specified" );

                ulPageId = 0;

                break;
            }
            else if( !hwndDlg )
            {
                PNBPAGE pnbp = WinSendMsg( hwndNB, BKM_QUERYPAGEDATA,
                                           MPFROMLONG( ulPageId ), NULL );

                if( pnbp == (PNBPAGE) BOOKERR_INVALID_PARAMETERS )
                {
                    Msg( "CheckDialogs QUERYPAGEDATA Invalid page specified" );

                    ulPageId = 0;

                    break;
                }
                else if( pnbp && pnbp->idDlg )
                {
                    hwndDlg = LoadAndAssociate( hwndNB, ulPageId, pnbp );

                    if( !hwndDlg )
                        ulPageId = 0;

                    break;
                }
            }
        }

        usWhat = BKA_NEXT;
    }

    // If all pages have been loaded, stop the timer

    if( !ulPageId )
        WinStopTimer( ANCHOR( hwndClient ), hwndClient, 1 );

    return;
}

/**********************************************************************/
/*----------------------- LoadAndAssociate ---------------------------*/
/*                                                                    */
/*  LOAD THE DIALOG BOX AND ASSOCIATE IT WITH A NOTEBOOK PAGE.        */
/*                                                                    */
/*  INPUT: notebook window handle,                                    */
/*         page id to associate dialog with,                          */
/*         pointer to info related this page                          */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: Dialog box window handle                                  */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/
static HWND LoadAndAssociate( HWND hwndNB, ULONG ulPageId, PNBPAGE pnbp )
{
    HWND hwndDlg, hwndClient = PARENT( hwndNB );

    hwndDlg = WinLoadDlg( hwndClient, hwndClient, pnbp->pfnwpDlg, 0,
                          pnbp->idDlg, NULL );

    if( hwndDlg )
    {
        // Associate the dialog with the page.

        if( !WinSendMsg( hwndNB, BKM_SETPAGEWINDOWHWND,
                         MPFROMP( ulPageId ), MPFROMLONG( hwndDlg ) ) )
        {
            WinDestroyWindow( hwndDlg );

            hwndDlg = NULLHANDLE;

            Msg( "BKM_SETPAGEWINDOWHWND RC(%X)", HWNDERR( hwndNB ));
        }
    }
    else
        Msg( "WinLoadDlg RC(%X)", HWNDERR( hwndNB ) );

    return hwndDlg;
}

/**********************************************************************/
/*------------------------------ wpPage ------------------------------*/
/*                                                                    */
/*  WINDOW PROCEDURE FOR ALL DIALOGS                                  */
/*                                                                    */
/*  INPUT: window handle, message id, message parameter 1 and 2.      */
/*                                                                    */
/*  1.                                                                */
/*                                                                    */
/*  OUTPUT: return code                                               */
/*--------------------------------------------------------------------*/
/**********************************************************************/
MRESULT EXPENTRY wpPage( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
    switch( msg )
    {
        case WM_COMMAND:

            return 0;
    }

    return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}

/**********************************************************************/
/*------------------------------- Msg --------------------------------*/
/*                                                                    */
/*  DISPLAY A MESSAGE TO THE USER.                                    */
/*                                                                    */
/*  INPUT: a message in printf format with its parms                  */
/*                                                                    */
/*  1. Format the message using vsprintf.                             */
/*  2. Sound a warning sound.                                         */
/*  3. Display the message in a message box.                          */
/*                                                                    */
/*  OUTPUT: nothing                                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/**********************************************************************/

#define MESSAGE_SIZE 1024

VOID Msg( PSZ szFormat,... )
{
    PSZ     szMsg;
    va_list argptr;

    if( (szMsg = (PSZ) malloc( MESSAGE_SIZE )) == NULL )
    {
        DosBeep( 1000, 1000 );

        return;
    }

    va_start( argptr, szFormat );

    vsprintf( szMsg, szFormat, argptr );

    va_end( argptr );

    szMsg[ MESSAGE_SIZE - 1 ] = 0;

    (void) WinAlarm( HWND_DESKTOP, WA_WARNING );

    (void) WinMessageBox(  HWND_DESKTOP, HWND_DESKTOP, szMsg,
                           PROGRAM_TITLE, 1, MB_OK | MB_MOVEABLE );

    free( szMsg );

    return;
}

/************************************************************************
 *                      E N D   O F   S O U R C E                       *
 ************************************************************************/
