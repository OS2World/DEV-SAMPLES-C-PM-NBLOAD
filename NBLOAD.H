/*********************************************************************
 *                                                                   *
 * MODULE NAME :  nbload.h               AUTHOR:  Rick Fishman       *
 * DATE WRITTEN:  10-18-92                                           *
 *                                                                   *
 * DESCRIPTION:                                                      *
 *                                                                   *
 *  Common definitions and function prototypes for NBLOAD.EXE        *
 *                                                                   *
 * HISTORY:                                                          *
 *                                                                   *
 *  10-18-92 - File Created                                          *
 *                                                                   *
 *  Rick Fishman                                                     *
 *  Code Blazers, Inc.                                               *
 *  4113 Apricot                                                     *
 *  Irvine, CA. 92720                                                *
 *  CIS ID: 72251,750                                                *
 *                                                                   *
 *********************************************************************/

#define ID_NBWINFRAME           1
#define ID_NB                   10

#define IDD_PAGE1               1000
#define EF_1                    1001

#define IDD_PAGE2               2000
#define IDD_PAGE2A              2100
#define EF_2A                   2101
#define IDD_PAGE2B              2200
#define EF_2B                   2201

#define IDD_PAGE3               3000
#define EF_3                    3001

#define IDD_PAGE4               4000
#define IDD_PAGE4A              4100
#define EF_4A                   4101
#define IDD_PAGE4B              4200
#define EF_4B                   4201
#define IDD_PAGE4C              4300
#define EF_4C                   4301
#define IDD_PAGE4D              4400
#define EF_4D                   4401

#define IDD_PAGE5               5000
#define EF_5                    5001

#define IDD_PAGE6               6000
#define IDD_PAGE6A              6100
#define EF_6A                   6101
#define IDD_PAGE6B              6200
#define EF_6B                   6201
#define IDD_PAGE6C              6300
#define EF_6C                   6301

#define IDD_PAGE71              7100
#define EF_71                   7101
#define IDD_PAGE72              7200
#define EF_72                   7201
#define IDD_PAGE73              7300
#define EF_73                   7301
#define IDD_PAGE74              7400
#define EF_74                   7401

#define DEBUG_FILENAME          "nbload.dbg"

#define NOTEBOOK_WINCLASS       "NoteBookSample"

#define PROGRAM_TITLE           "Notebook Control Sample Loading Program"

#define HABERR( hab )           (ERRORIDERROR( WinGetLastError( hab ) ))

#define ANCHOR( hwnd )          (WinQueryAnchorBlock( hwnd ))

#define HWNDERR( hwnd )         \
            (ERRORIDERROR( WinGetLastError( WinQueryAnchorBlock( hwnd ) ) ))

#define INSTDATA( hwnd )        (WinQueryWindowPtr( hwnd, 0 ))

#define PARENT( hwnd )          (WinQueryWindow( hwnd, QW_PARENT ))

#define GRANDPARENT( hwnd )   (WinQueryWindow( WinQueryWindow(hwnd, QW_PARENT)))

typedef struct _NBPAGE              // VARIABLES USED FOR A NOTEBOOK PAGE
{
    PFNWP    pfnwpDlg;              // Window procedure address for the dialog
    PSZ      szStatusLineText;      // Text to go on status line
    PSZ      szTabText;             // Text to go on major tab
    ULONG    idDlg;                 // ID of the dialog box for this page
    ULONG    idFocus;               // ID of the control to get the focus first
    BOOL     fParent;               // Is this a Parent page with minor pages
    USHORT   usTabType;             // BKA_MAJOR or BKA_MINOR

} NBPAGE, *PNBPAGE;

/****************************************************************************
 *                        E N D   O F   S O U R C E                         *
 ****************************************************************************/
