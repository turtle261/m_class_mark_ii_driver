/*
 * "$Id: rastertodpl.c $"
 *
 *   DPL Label printer filter for CUPS.
 *
 *   Copyright 2007-2012 by Apple Inc.
 *   Copyright 2001-2007 by Easy Software Products.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Apple Inc. and are protected by Federal copyright
 *   law.  Distribution and use rights are outlined in the file "LICENSE.txt"
 *   which should have been included with this file. 
 *   file is missing or damaged, see the license at "http://www.cups.org/".
 *
 *   This file is subject to the Apple OS-Developed Software exception.
 */

#define _(x) x

#include <cups/cups.h>
#include <cups/ppd.h>
#include <cups/raster.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>
#include <errno.h>

/*
 * This driver filter currently supports Datamax I Series label printers.
 *
 * The driver has been tested with a Datamax I-4206 Label Printer but
 * may work with other models that support DPL. This filter is based
 * on the rastertolabel. It uses the
 * PCX file format to send the raster output to Module D (DRAM) of the
 * label printer. Then a label format is created using that image. All the
 * printer commands used are documented from Datamax's DPL Programming Manual.
 */


/* Typedefs for pcx file header */

typedef struct _rgb_triple_s 
{
	unsigned char red; 		/* Red color */
	unsigned char green;	/* Green color */
	unsigned char blue;		/* Blue color */
} _rgb_triple_t;

typedef struct _pcx_header_s 
{
	unsigned char manufacturer;		/* 0x0a = ZSoft */
	unsigned char version;				/* Version 5 */
	unsigned char encoding;				/* 1 = .PCX RLE */
	unsigned char bitsperpixel;		/* 1,2,4, or 8 */
	unsigned short int xul;				/* Upper left corner horz pos */
	unsigned short int yul;				/* Upper left corner vert pos */
	unsigned short int xlr;				/* Lower right corner horz pos */
	unsigned short int ylr;				/* Lower right corner vert pos */
	unsigned short int hdpi;			/* horz dot per inch */
	unsigned short int vdpi;			/* vert dot per inch */
	_rgb_triple_t colormap[16];		/* colormap 16 color triples */
	unsigned char reserved;				/* set to zero (old video mode) */
	unsigned char nplanes;				/* number of color planes = 1 */
	unsigned short int bytesperline;	/* bytes per line (even number) */
	unsigned short int paletteinfo;		/* 0x01 (color | b&w) 0x02 (grayscale) */
	unsigned short int hscreensize;		/* horz screen size in pixels */
	unsigned short int vscreensize;		/* vert screen size in pixels */
	unsigned char filler[54];			/* blank space fill with zeroes */
} _pcx_header_t;

/* pcx functions */

void pcx_write_line
(
  unsigned char *linep, /* Scan line buffer pointer */
  int length,           /* Scan line buffer length (in bytes) */
  FILE *fp              /* PCX file pointer */
);

void pcx_encode
(
  int data,     /* Data byte */
  int count,    /* Data byte repeat count */
  FILE *fp      /* PCX file pointer */
);

/*
 * Model number constants...
 */

#define A_SERIES 65		/* Datamax DPL based printers */
#define E_SERIES 69
#define I_SERIES 73
#define M_SERIES 77
#define W_SERIES 87

/*
 * Defines to make the commands easier to read...
 */

#define SOH 1
#define STX 2
#define CR 13
#define ESC 27

/*
 * Globals...
 */

unsigned char	*Buffer;		/* Output buffer */
int ModelNumber,					/* cupsModelNumber attribute */
		Page,							/* Current page */
		Feed,							/* Number of lines to skip */
		Canceled;						/* Non-zero if job is canceled */
_pcx_header_t	pcx;				/* PCX header for .pcx file format */

/*
 * Prototypes...
 */

void	Setup(ppd_file_t *ppd);
void	StartPage(ppd_file_t *ppd, cups_page_header2_t *header);
void	EndPage(ppd_file_t *ppd, cups_page_header2_t *header);
void	CancelJob(int sig);
void	OutputLine(ppd_file_t *ppd, cups_page_header2_t *header, int y);

/*
 * 'Setup()' - Prepare the printer for printing.
 */

void
Setup(ppd_file_t *ppd)			/* I - PPD file */
{

 /*
  * Get the model number from the PPD file...
  */

  if (ppd)
    ModelNumber = ppd->model_number;

  fprintf(stderr,"ModelNumber is %d\n",ModelNumber);

  /* Command reset */
//  printf("%c#",SOH);

 /*
  * Initialize based on the model number...
  */

  switch (ModelNumber)
  {
    case E_SERIES :
				/*
				 * Start PCX graphics send <SOH>D then <STX>IAP<CR>
				 * We must flip the image in the Datamax since it would print mirror otherwise.
				 */
				 
				printf("%cD%cIAPCups%c",SOH,STX,CR);
        break;
    case M_SERIES :
        /* M-Class Mark II printers use Module D for DRAM */
        printf("%cD%cIDPCups%c",SOH,STX,CR);
        break;
    default :
    
				/*
        * Start PCX graphics send <SOH>D then <STX>ICP<CR>
				* We must flip the image in the Datamax since it would print mirror otherwise.
				*/
				
				printf("%cD%cICPCups%c",SOH,STX,CR);
  }
}

/*
 * 'StartPage()' - Start a page of graphics.
 */

void
StartPage(ppd_file_t         *ppd,			/* I - PPD file */
          cups_page_header2_t *header)	/* I - Page header */
{
   char *ptr;
   int i;

 /*
  * Show page device dictionary...
  */

  fprintf(stderr, "DEBUG: StartPage...\n");
  fprintf(stderr, "DEBUG: MediaClass = \"%s\"\n", header->MediaClass);
  fprintf(stderr, "DEBUG: MediaColor = \"%s\"\n", header->MediaColor);
  fprintf(stderr, "DEBUG: MediaType = \"%s\"\n", header->MediaType);
  fprintf(stderr, "DEBUG: OutputType = \"%s\"\n", header->OutputType);

  fprintf(stderr, "DEBUG: AdvanceDistance = %d\n", header->AdvanceDistance);
  fprintf(stderr, "DEBUG: AdvanceMedia = %d\n", header->AdvanceMedia);
  fprintf(stderr, "DEBUG: Collate = %d\n", header->Collate);
  fprintf(stderr, "DEBUG: CutMedia = %d\n", header->CutMedia);
  fprintf(stderr, "DEBUG: Duplex = %d\n", header->Duplex);
  fprintf(stderr, "DEBUG: HWResolution = [ %d %d ]\n", header->HWResolution[0],
          header->HWResolution[1]);
  fprintf(stderr, "DEBUG: ImagingBoundingBox = [ %d %d %d %d ]\n",
          header->ImagingBoundingBox[0], header->ImagingBoundingBox[1],
          header->ImagingBoundingBox[2], header->ImagingBoundingBox[3]);
  fprintf(stderr, "DEBUG: InsertSheet = %d\n", header->InsertSheet);
  fprintf(stderr, "DEBUG: Jog = %d\n", header->Jog);
  fprintf(stderr, "DEBUG: LeadingEdge = %d\n", header->LeadingEdge);
  fprintf(stderr, "DEBUG: Margins = [ %d %d ]\n", header->Margins[0],
          header->Margins[1]);
  fprintf(stderr, "DEBUG: ManualFeed = %d\n", header->ManualFeed);
  fprintf(stderr, "DEBUG: MediaPosition = %d\n", header->MediaPosition);
  fprintf(stderr, "DEBUG: MediaWeight = %d\n", header->MediaWeight);
  fprintf(stderr, "DEBUG: MirrorPrint = %d\n", header->MirrorPrint);
  fprintf(stderr, "DEBUG: NegativePrint = %d\n", header->NegativePrint);
  fprintf(stderr, "DEBUG: NumCopies = %d\n", header->NumCopies);
  fprintf(stderr, "DEBUG: Orientation = %d\n", header->Orientation);
  fprintf(stderr, "DEBUG: OutputFaceUp = %d\n", header->OutputFaceUp);
  fprintf(stderr, "DEBUG: PageSize = [ %d %d ]\n", header->PageSize[0],
          header->PageSize[1]);
  fprintf(stderr, "DEBUG: Separations = %d\n", header->Separations);
  fprintf(stderr, "DEBUG: TraySwitch = %d\n", header->TraySwitch);
  fprintf(stderr, "DEBUG: Tumble = %d\n", header->Tumble);
  fprintf(stderr, "DEBUG: cupsWidth = %d\n", header->cupsWidth);
  fprintf(stderr, "DEBUG: cupsHeight = %d\n", header->cupsHeight);
  fprintf(stderr, "DEBUG: cupsMediaType = %d\n", header->cupsMediaType);
  fprintf(stderr, "DEBUG: cupsBitsPerColor = %d\n", header->cupsBitsPerColor);
  fprintf(stderr, "DEBUG: cupsBitsPerPixel = %d\n", header->cupsBitsPerPixel);
  fprintf(stderr, "DEBUG: cupsBytesPerLine = %d\n", header->cupsBytesPerLine);
  fprintf(stderr, "DEBUG: cupsColorOrder = %d\n", header->cupsColorOrder);
  fprintf(stderr, "DEBUG: cupsColorSpace = %d\n", header->cupsColorSpace);
  fprintf(stderr, "DEBUG: cupsCompression = %d\n", header->cupsCompression);
  fprintf(stderr, "DEBUG: cupsRowCount = %d\n", header->cupsRowCount);
  fprintf(stderr, "DEBUG: cupsRowFeed = %d\n", header->cupsRowFeed);
  fprintf(stderr, "DEBUG: cupsRowStep = %d\n", header->cupsRowStep);

  pcx.manufacturer = 0x0a;					/* 0x0a = ZSoft */
  pcx.version = 2;							/* Version 2 */
  pcx.encoding = 1;							/* 1 = .PCX RLE */
  pcx.bitsperpixel = header->cupsBitsPerPixel;	/* 1,2,4, or 8 */
  pcx.xul = 0;								/* Upper left corner horz pos */
  pcx.yul = 0;								/* Upper left corner vert pos */
  pcx.xlr = header->cupsWidth - 1;		/* Lower right corner horz pos */
  pcx.ylr = header->cupsHeight - 1;		/* Lower right corner vert pos */
  pcx.hdpi = header->HWResolution[0];	/* horz dot per inch */
  pcx.vdpi = header->HWResolution[1];	/* vert dot per inch */

  pcx.colormap[0].red = 0;			/* Black is White */
  pcx.colormap[0].green = 0;
  pcx.colormap[0].blue = 0;
  pcx.colormap[1].red = 255;		/* White is Black */
  pcx.colormap[1].green = 255;
  pcx.colormap[1].blue = 255;

  for (i = 0; i < 14; i++)
  {
		pcx.colormap[i+2].red = 0;
		pcx.colormap[i+2].green = 0;
		pcx.colormap[i+2].blue = 0;
  }

  pcx.reserved = 0;				/* set to zero (old video mode) */
  pcx.nplanes = 1;				/* number of color planes = 1 */
  pcx.bytesperline = header->cupsBytesPerLine;	/* bytes per line (even number) */
  pcx.paletteinfo = 0x01;			/* 0x01 (color | b&w) 0x02 (grayscale) */
  pcx.hscreensize = header->cupsWidth;		/* horz screen size in pixels */
  pcx.vscreensize = header->cupsHeight;		/* vert screen size in pixels */
  for (i = 0; i < 54; i++)
  {
		pcx.filler[i] = 0;			/* blank space fill with zeroes */
  }

  ptr = (char *) &pcx;

  for (i = 0; i < sizeof(_pcx_header_t); i++)
  {
		fputc(*ptr++,stdout);
  }
 
  Buffer = malloc(header->cupsBytesPerLine);
  Feed   = 0;
}


/*
 * 'EndPage()' - Finish a page of graphics.
 */

void
EndPage(ppd_file_t *ppd,		/* I - PPD file */
        cups_page_header2_t *header)	/* I - Page header */
{
  static const char *val = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcde";
  int max_page_length = 0;
  ppd_choice_t	*choice;

  printf("%c",CR);

  max_page_length = (header->PageSize[1] / 72 * 3 * 254) >= 500 ? (header->PageSize[1] / 72 * 3 * 254) : 500;
  printf("%cM%04d", STX, max_page_length);

  if ((int)header->AdvanceDistance >= 0)
  {
    printf("%c%ct1", STX, ESC);

    if ((int)header->AdvanceDistance >= 1000)
    {
      printf("%cKf%04d", STX, (int)header->AdvanceDistance);
    } else {
      printf("%cf%03d", STX, (int)header->AdvanceDistance);
    }
  }

  if ((choice = ppdFindMarkedChoice(ppd, "MediaClass")) != NULL)
  {
		if (strcmp("LablesEdge", choice->choice) == 0)
		{
			printf("%cc0000", STX);
			printf("%ce", STX);
		}

		if (strcmp("Continuous", choice->choice) == 0)
		{
			printf("%cc%04d", STX, header->PageSize[1] / 72 * 254);
		}

		if (strcmp("LablesMark", choice->choice) == 0)
		{
			printf("%cc0000", STX);
			printf("%cr", STX);
		}
  }
   
  printf("%cL%c",STX,CR);
  printf("m%c",CR);
  printf("D11%c",CR);
  printf("H%02d%c", header->cupsCompression ,CR);

  if ((choice = ppdFindMarkedChoice(ppd, "DPLPrintSpeed")) != NULL)
		printf("P%c%c", val[atoi(choice->choice)],CR);
   
  if ((choice = ppdFindMarkedChoice(ppd, "DPLFeedSpeed")) != NULL)
		printf("S%c%c", val[atoi(choice->choice)],CR);
   
  if ((choice = ppdFindMarkedChoice(ppd, "DPLReverseSpeed")) != NULL)
		printf("p%c%c", val[atoi(choice->choice)],CR);
   
  if ((choice = ppdFindMarkedChoice(ppd, "DPLRowOffset")) != NULL)
		printf("R%04d%c", atoi(choice->choice),CR);

  if ((choice = ppdFindMarkedChoice(ppd, "DPLColumnOffset")) != NULL)
    printf("C%04d%c", atoi(choice->choice),CR);

   printf("Q%04d%c", header->NumCopies, CR);
   
  printf("1Y1100000000000Cups%c",CR);
   
  printf("E%c",CR);

  switch (ModelNumber)
  {
    case E_SERIES :
				printf("%cxAGCups%c",STX,CR);
				printf("%czA",STX);
        break;
    case M_SERIES :
        printf("%cxDGCups%c",STX,CR);
        printf("%czD",STX);
        break;
    default :
				printf("%cxCGCups%c",STX,CR);
				printf("%czC",STX);
  }

  fflush(stdout);
  
  free(Buffer);
}


/*
 * 'CancelJob()' - Cancel the current job...
 */

void
CancelJob(int sig)			/* I - Signal */
{
  (void)sig;

  Canceled = 1;
}

void
OutputLine
(
ppd_file_t         *ppd,	/* I - PPD file */
cups_page_header2_t *header,	/* I - Page header */
int                y		/* I - Line number */
)	
{
	pcx_write_line(Buffer, header->cupsBytesPerLine,stdout);
}



/*
 * 'main()' - Main entry and processing of driver.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  int			fd;		/* File descriptor */
  cups_raster_t		*ras;		/* Raster stream for printing */
  cups_page_header2_t	header;		/* Page header from file */
  int			y;		/* Current line */
  ppd_file_t		*ppd;		/* PPD file */
  int			num_options;	/* Number of options */
  cups_option_t		*options;	/* Options */
#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
  struct sigaction action;		/* Actions for POSIX signals */
#endif /* HAVE_SIGACTION && !HAVE_SIGSET */

  /* -------------------------------------------------------------------- */
  /* Redirect stderr to a writable log file for easier debugging.         */
  /* The Makefile installation step ensures the file exists and is world  */
  /* writable, so the filter (usually running as the unprivileged 'lp'    */
  /* user) can append to it.                                              */
  /* -------------------------------------------------------------------- */

#define RASTERDPL_LOG_PATH "/home/theo/log.txt"

  {
    FILE *log_fp = fopen(RASTERDPL_LOG_PATH, "a");
    if (log_fp)
    {
      /* Duplicate log file descriptor to stderr so all existing debug
         messages continue to work without modification. */
      dup2(fileno(log_fp), STDERR_FILENO);
      /* Ensure line-buffered output for easier tail-f debugging. */
      setvbuf(stderr, NULL, _IOLBF, 0);
    }
  }

  if (argc < 6 || argc > 7)
  {
    fprintf(stderr,
                    _("Usage: %s job-id user title copies options [file]\n"),
                    "rastertodpl");
    return (1);
  }

  if (argc == 7)
  {
    if ((fd = open(argv[6], O_RDONLY)) == -1)
    {
      fprintf(stderr, _("ERROR: Unable to open raster file - %s\n"),
                      strerror(errno));
      sleep(1);
      return (1);
    }
  }
  else
    fd = 0;

  ras = cupsRasterOpen(fd, CUPS_RASTER_READ);

  Canceled = 0;

#ifdef HAVE_SIGSET
  sigset(SIGTERM, CancelJob);
#elif defined(HAVE_SIGACTION)
  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_handler = CancelJob;
  sigaction(SIGTERM, &action, NULL);
#else
  signal(SIGTERM, CancelJob);
#endif

  num_options = cupsParseOptions(argv[5], 0, &options);

  if ((ppd = ppdOpenFile(getenv("PPD"))) != NULL)
  {
    ppdMarkDefaults(ppd);
    cupsMarkOptions(ppd, num_options, options);
  }

  Page      = 0;
  Canceled = 0;

  while (cupsRasterReadHeader2(ras, &header))
  {
    if (Canceled)
      break;

    Page ++;

    Setup(ppd);

    fprintf(stderr, "PAGE: %d 1\n", Page);

    StartPage(ppd, &header);

    for (y = 0; y < header.cupsHeight && !Canceled; y ++)
    {
      if (Canceled)
				break;

      if ((y & 15) == 0)
        cupsFilePrintf(cupsFileStderr(), _("INFO: Printing page %d, %d%% complete...\n"),
                        Page, 100 * y / header.cupsHeight);

      if (cupsRasterReadPixels(ras, Buffer, header.cupsBytesPerLine) < 1)
        break;

      OutputLine(ppd, &header, y);
    }

    EndPage(ppd, &header);

    if (Canceled)
      break;
  }

  cupsRasterClose(ras);
  if (fd != 0)
    close(fd);

  ppdClose(ppd);
  cupsFreeOptions(num_options, options);

  if (Page == 0)
  {
    fputs(_("ERROR: No pages found!\n"), stderr);
    return (1);
  }
  else
  {
    fputs(_("INFO: Ready to print.\n"), stderr);
    return (0);
  }

  return (Page == 0);
}
 
void pcx_write_line
(
  unsigned char *linep, /* Scan line buffer pointer */
  int length,           /* Scan line buffer length (in bytes) */
  FILE *fp              /* PCX file pointer */
)
{
  int curr_data;
  int prev_data;
  int data_count;
  int line_count;

  prev_data = *linep++;
  data_count = 1;
  line_count = 1;

  while (line_count < length)
  {
    curr_data = *linep++;
    line_count++;

    if (curr_data == prev_data)
    {
      data_count++;

      if (data_count == 0x3f)
      {
        pcx_encode(prev_data, data_count, fp);
        data_count = 0;
      }
    }
    else
    {
      if (data_count > 0)
        pcx_encode(prev_data, data_count, fp);

      prev_data = curr_data;
      data_count = 1;
    }
  }

  if (data_count > 0)
  {
    pcx_encode(prev_data, data_count, fp);
  }
}

void pcx_encode
(
  int data,     /* Data byte */
  int count,    /* Data byte repeat count */
  FILE *fp      /* PCX file pointer */
)
{
  if (((~data & 0xc0) == 0xc0) || count > 1)
  {
    putc(0xc0 | count, fp);
  }

  putc(~data, fp);
}