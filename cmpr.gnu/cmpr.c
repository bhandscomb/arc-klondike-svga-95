/********************************************/
/*  SIXPACK.C -- Data compression program   */
/*  Written by Philip G. Gage, April 1991   */
/********************************************/

/* Klondikified by Brian 'Aardvark' Handscomb, July 1996 */


#include <stdio.h>
#include <malloc.h>        /* Use <malloc.c> for Power C */



/*** BEGIN KLN ***/

/* GNU knows no far, use standard malloc/free instead of far... versions */
#define far
#define farmalloc malloc
#define farfree   free


/* we use this a fair bit */

typedef unsigned long ULONG;


/* input/output */

FILE *ifp=NULL,*ofp=NULL;
int inbytesremaining=0;

/* tag at start of file */

char PackTag[]="81T!";


/* misc - compressflag MUST be non-zero here, zero for decompressor */

int compressflag=1;
ULONG checksum=0;


/* in compression code, getc/putc changed to getbyte/putbyte (see below) */

int getbyte(FILE *fp) {
  int c;
  if(inbytesremaining) {
    inbytesremaining--;
    c=getc(fp);
    if(compressflag)
      checksum =
        ((checksum<<5)|(checksum>>27)) ^
        (((ULONG)c)|((ULONG)c<<11)|((ULONG)c<<23))
        ;
    return(c);
    }
  else
    return(EOF);
  }

void putbyte(int c,FILE *fp) {
  if(!compressflag)
    checksum =
      ((checksum<<5)|(checksum>>27)) ^
      (((ULONG)c)|((ULONG)c<<11)|((ULONG)c<<23))
      ;
  putc(c,fp);
  }



/*** END KLN ***/



#define TEXTSEARCH 1000   /* Max strings to search in text file */
#define BINSEARCH   200   /* Max strings to search in binary file */
#define TEXTNEXT     50   /* Max search at next character in text file */
#define BINNEXT      20   /* Max search at next character in binary file */
#define MAXFREQ    2000   /* Max frequency count before table reset */ 
#define MINCOPY       3   /* Shortest string copy length */
#define MAXCOPY      64   /* Longest string copy length */
#define SHORTRANGE    3   /* Max distance range for shortest length copy */
#define COPYRANGES    6   /* Number of string copy distance bit ranges */
short copybits[COPYRANGES] = {4,6,8,10,12,14};   /* Distance bits */

#define CODESPERRANGE (MAXCOPY - MINCOPY + 1)
int copymin[COPYRANGES], copymax[COPYRANGES];
int maxdistance, maxsize;
int distance, insert = MINCOPY, dictfile = 0, binary = 0;

#define NIL -1                    /* End of linked list marker */
#define HASHSIZE 16384            /* Number of entries in hash table */
#define HASHMASK (HASHSIZE - 1)   /* Mask for hash key wrap */
short far *head, far *tail;       /* Hash table */
short far *succ, far *pred;       /* Doubly linked lists */
unsigned char *buffer;            /* Text buffer */

/* Define hash key function using MINCOPY characters of string prefix */
#define getkey(n) ((buffer[n] ^ (buffer[(n+1)%maxsize]<<4) ^ \
                   (buffer[(n+2)%maxsize]<<8)) & HASHMASK)

/* Adaptive Huffman variables */
#define TERMINATE 256             /* EOF code */
#define FIRSTCODE 257             /* First code for copy lengths */
#define MAXCHAR (FIRSTCODE+COPYRANGES*CODESPERRANGE-1)
#define SUCCMAX (MAXCHAR+1)
#define TWICEMAX (2*MAXCHAR+1)
#define ROOT 1
short left[MAXCHAR+1], right[MAXCHAR+1];  /* Huffman tree */
short up[TWICEMAX+1], freq[TWICEMAX+1];

/*** Bit packing routines ***/

int input_bit_count = 0;           /* Input bits buffered */
int input_bit_buffer = 0;          /* Input buffer */
int output_bit_count = 0;          /* Output bits buffered */
int output_bit_buffer = 0;         /* Output buffer */
long bytes_in = 0, bytes_out = 0;  /* File size counters */

/* Write one bit to output file */
output_bit(output,bit)
  FILE *output;
  int bit;
{
  output_bit_buffer <<= 1;
  if (bit) output_bit_buffer |= 1;
  if (++output_bit_count == 8) {
    putbyte(output_bit_buffer,output);
    output_bit_count = 0;
    ++bytes_out;
  }
}

/* Read a bit from input file */
int input_bit(input)
  FILE *input;
{
  int bit;

  if (input_bit_count-- == 0) {
    input_bit_buffer = getbyte(input);
    if (input_bit_buffer == EOF) {
      printf(" UNEXPECTED END OF FILE\n");
      exit(1);
    }
    ++bytes_in;
    input_bit_count = 7;
  }
  bit = (input_bit_buffer & 0x80) != 0;
  input_bit_buffer <<= 1;
  return(bit);
}

/* Write multibit code to output file */
output_code(output,code,bits)
  FILE *output;
  int code,bits;
{
  int i;

  for (i = 0; i<bits; i++) {
    output_bit(output,code & 0x01);
    code >>= 1;
  }
}

/* Read multibit code from input file */
int input_code(input,bits)
  FILE *input;
  int bits;
{
  int i, bit = 1, code = 0;

  for (i = 0; i<bits; i++) {
    if (input_bit(input)) code |= bit;
    bit <<= 1;
  }
  return(code);
}

/* Flush any remaining bits to output file before closing file */
flush_bits(output)
  FILE *output;
{
  if (output_bit_count > 0) {
    putbyte((output_bit_buffer << (8-output_bit_count)),output);
    ++bytes_out;
  }
}

/*** Adaptive Huffman frequency compression ***/

/* Data structure based partly on "Application of Splay Trees
   to Data Compression", Communications of the ACM 8/88 */

/* Initialize data for compression or decompression */
initialize()
{
  int i, j;

  /* Initialize Huffman frequency tree */
  for (i = 2; i<=TWICEMAX; i++) {
    up[i] = i/2;
    freq[i] = 1;
  }
  for (i = 1; i<=MAXCHAR; i++) {
    left[i] = 2*i;
    right[i] = 2*i+1;
  }

  /* Initialize copy distance ranges */
  j = 0;
  for (i = 0; i<COPYRANGES; i++) {
    copymin[i] = j;
    j += 1 << copybits[i];
    copymax[i] = j - 1;
  }
  maxdistance = j - 1;
  maxsize = maxdistance + MAXCOPY;
}

/* Update frequency counts from leaf to root */
update_freq(a,b)
  int a,b;
{
  do {
    freq[up[a]] = freq[a] + freq[b];
    a = up[a];
    if (a != ROOT) {
      if (left[up[a]] == a) b = right[up[a]];
      else b = left[up[a]];
    }
  } while (a != ROOT);

  /* Periodically scale frequencies down by half to avoid overflow */
  /* This also provides some local adaption and better compression */
  if (freq[ROOT] == MAXFREQ)
    for (a = 1; a<=TWICEMAX; a++) freq[a] >>= 1;
}

/* Update Huffman model for each character code */
update_model(code)
  int code;
{
  int a, b, c, ua, uua;

  a = code + SUCCMAX;
  ++freq[a];
  if (up[a] != ROOT) {
    ua = up[a];
    if (left[ua] == a) update_freq(a,right[ua]);
    else update_freq(a,left[ua]);
    do {
      uua = up[ua];
      if (left[uua] == ua) b = right[uua];
      else b = left[uua];

      /* If high freq lower in tree, swap nodes */
      if (freq[a] > freq[b]) {
        if (left[uua] == ua) right[uua] = a;
        else left[uua] = a;
        if (left[ua] == a) {
          left[ua] = b; c = right[ua];
        } else {
          right[ua] = b; c = left[ua];
        }
        up[b] = ua; up[a] = uua;
        update_freq(b,c); a = b;
      }
      a = up[a]; ua = up[a];
    } while (ua != ROOT);
  }
}

/* Compress a character code to output stream */
compress(output,code)
  FILE *output;
  int code;
{
  int a, sp = 0;
  int stack[50];

  a = code + SUCCMAX;
  do {
    stack[sp++] = (right[up[a]] == a);
    a = up[a];
  } while (a != ROOT);
  do {
    output_bit(output,stack[--sp]);
  } while (sp);
  update_model(code);
}

/* Uncompress a character code from input stream */
int uncompress(input)
  FILE *input;
{
  int a = ROOT;

  do {
    if (input_bit(input)) a = right[a];
    else a = left[a];
  } while (a <= MAXCHAR);
  update_model(a-SUCCMAX);
  return(a-SUCCMAX);
}

/*** Hash table linked list string search routines ***/

/* Add node to head of list */
add_node(n)  
  int n;
{
  int key;

  key = getkey(n);
  if (head[key] == NIL) {
    tail[key] = n;
    succ[n] = NIL;
  } else {
    succ[n] = head[key];
    pred[head[key]] = n;
  }
  head[key] = n;
  pred[n] = NIL;
}

/* Delete node from tail of list */
delete_node(n)
  int n;
{
  int key;

  key = getkey(n);
  if (head[key] == tail[key])
    head[key] = NIL;
  else {
    succ[pred[tail[key]]] = NIL;
    tail[key] = pred[tail[key]];
  }
}

/* Find longest string matching lookahead buffer string */
int match(n,depth)
  int n,depth;
{
  int i, j, index, key, dist, len, best = 0, count = 0;

  if (n == maxsize) n = 0;
  key = getkey(n);
  index = head[key];
  while (index != NIL) {
    if (++count > depth) break;     /* Quit if depth exceeded */
    if (buffer[(n+best)%maxsize] == buffer[(index+best)%maxsize]) {
      len = 0;  i = n;  j = index;
      while (buffer[i]==buffer[j] && len<MAXCOPY && j!=n && i!=insert) {
        ++len;
        if (++i == maxsize) i = 0;
        if (++j == maxsize) j = 0;
      }
      dist = n - index;
      if (dist < 0) dist += maxsize;
      dist -= len;
      /* If dict file, quit at shortest distance range */
      if (dictfile && dist > copymax[0]) break;
      if (len > best && dist <= maxdistance) {     /* Update best match */
        if (len > MINCOPY || dist <= copymax[SHORTRANGE+binary]) {
          best = len; distance = dist;
        }
      }
    }
    index = succ[index];
  }
  return(best);
}

/*** Finite Window compression routines ***/

#define IDLE 0    /* Not processing a copy */
#define COPY 1    /* Currently processing copy */

/* Check first buffer for ordered dictionary file */
/* Better compression using short distance copies */
dictionary()
{
  int i = 0, j = 0, k, count = 0;

  /* Count matching chars at start of adjacent lines */
  while (++j < MINCOPY+MAXCOPY) {
    if (buffer[j-1] == 10) {
      k = j;
      while (buffer[i++] == buffer[k++]) ++count;
      i = j;
    }
  }
  /* If matching line prefixes > 25% assume dictionary */
  if (count > (MINCOPY+MAXCOPY)/4) dictfile = 1;
}

/* Encode file from input to output */
encode(input,output)
  FILE *input, *output;
{
  int c, i, n=MINCOPY, addpos=0, len=0, full=0, state=IDLE, nextlen;

  initialize();
  head = farmalloc((unsigned long)HASHSIZE*sizeof(short));
  tail = farmalloc((unsigned long)HASHSIZE*sizeof(short));
  succ = farmalloc((unsigned long)maxsize*sizeof(short));
  pred = farmalloc((unsigned long)maxsize*sizeof(short));
  buffer = (unsigned char *) malloc(maxsize*sizeof(unsigned char));
  if (head==NULL || tail==NULL || succ==NULL || pred==NULL || buffer==NULL) {
    printf("Error allocating memory\n");
    exit(1);
  }

  /* Initialize hash table to empty */
  for (i = 0; i<HASHSIZE; i++) {
    head[i] = NIL;
  }

  /* Compress first few characters using Huffman */
  for (i = 0; i<MINCOPY; i++) {
    if ((c = getbyte(input)) == EOF) {
      compress(output,TERMINATE);
      flush_bits(output);
      return(bytes_in);
    }
    compress(output,c);  ++bytes_in;
    buffer[i] = c;
  }

  /* Preload next few characters into lookahead buffer */
  for (i = 0; i<MAXCOPY; i++) {
    if ((c = getbyte(input)) == EOF) break;
    buffer[insert++] = c;  ++bytes_in;
    if (c > 127) binary = 1;     /* Binary file ? */
  }
  dictionary();  /* Check for dictionary file */

  while (n != insert) {
    /* Check compression to insure really a dictionary file */
    if (dictfile && ((bytes_in % MAXCOPY) == 0))
      if (bytes_in/bytes_out < 2)
        dictfile = 0;     /* Oops, not a dictionary file ! */

    /* Update nodes in hash table lists */
    if (full) delete_node(insert);
    add_node(addpos);

    /* If doing copy, process character, else check for new copy */
    if (state == COPY) {
      if (--len == 1) state = IDLE;
    } else {

      /* Get match length at next character and current char */
      if (binary) {
        nextlen = match(n+1,BINNEXT);
        len = match(n,BINSEARCH);
      } else {
        nextlen = match(n+1,TEXTNEXT);
        len = match(n,TEXTSEARCH);
      }

      /* If long enough and no better match at next char, start copy */
      if (len >= MINCOPY && len >= nextlen) {
        state = COPY;

        /* Look up minimum bits to encode distance */
        for (i = 0; i<COPYRANGES; i++) {
          if (distance <= copymax[i]) {
            compress(output,FIRSTCODE-MINCOPY+len+i*CODESPERRANGE);
            output_code(output,distance-copymin[i],copybits[i]);
            break;
          }
        }
      }
      else   /* Else output single literal character */
        compress(output,buffer[n]);
    }

    /* Advance buffer pointers */
    if (++n == maxsize) n = 0;
    if (++addpos == maxsize) addpos = 0;

    /* Add next input character to buffer */
    if (c != EOF) {
      if ((c = getbyte(input)) != EOF) {
        buffer[insert++] = c;  ++bytes_in;
      } else full = 0;
      if (insert == maxsize) {
        insert = 0; full = 1;
      }
    }
  }

  /* Output EOF code and free memory */
  compress(output,TERMINATE);
  flush_bits(output);
  farfree(head); farfree(tail); farfree(succ); farfree(pred);
  free(buffer);
}

/* Decode file from input to output */
decode(input,output)
  FILE *input,*output;
{
  int c, i, j, k, dist, len, n = 0, index;

  initialize();
  buffer = (unsigned char *) malloc(maxsize*sizeof(unsigned char));
  if (buffer == NULL) {
    printf("Error allocating memory\n");
    exit(1);
  }
  while ((c = uncompress(input)) != TERMINATE) {
    if (c < 256) {     /* Single literal character ? */
      putbyte(c,output);
      ++bytes_out;
      buffer[n++] = c;
      if (n == maxsize) n = 0;
    } else {            /* Else string copy length/distance codes */
      index = (c - FIRSTCODE)/CODESPERRANGE;
      len = c - FIRSTCODE + MINCOPY - index*CODESPERRANGE;
      dist = input_code(input,copybits[index]) + len + copymin[index];
      j = n; k = n - dist;
      if (k < 0) k += maxsize;
      for (i = 0; i<len; i++) {
        putbyte(buffer[k],output);  ++bytes_out;
        buffer[j++] = buffer[k++];
        if (j == maxsize) j = 0;
        if (k == maxsize) k = 0;
      }
      n += len;
      if (n >= maxsize) n -= maxsize;
    }
  }
  free(buffer);
}



/*** BEGIN KLN ***/



/* error handler */

void err(char *msg) {
  if(ifp) fclose(ifp);
  if(ofp) fclose(ofp);
  puts(msg);
  exit(1);
  }


/* wrapper for fwrite() */

int FWRITE(void *buff,int len,int items,FILE *fp) {
  int rc;
  if((rc=fwrite(buff,len,items,fp))!=items)
    err("Error writing file");
  return(rc);
  }


/* the main() event :-) */

int main(int argc,char *argv[]) {
  char outname[100];

  /* check args, make out file name if needed */

  if((argc!=3)&&(argc!=2))
    err("Usage: cmpr <infile> [outfile]");
  if(argc==3)
    strcpy(outname,argv[2]);
  else
    sprintf(outname,"%.*s_",(int)(strlen(argv[1])-1),argv[1]);

  /* report filenames */

  printf("%s ==> %s : ",argv[1],outname);
  fflush(stdout);

  /* open the files */

  if((ifp=fopen(argv[1],"rb"))==NULL)
    err("Unable to open input file");
  if((ofp=fopen(outname,"w+b"))==NULL)
    err("Unable to open output file");

  /*
  ** FILE HEADER:
  **   char Tag[4];      PackTag[]
  **   ULONG total_in;   total input bytes
  **   ULONG total_out;  total encoded bytes (excl. headers, etc)
  **   ULONG checksum;   checksum of original bytes
  **
  ** we don't know the totals or checksum yet, we'll update them later
  */

  FWRITE(PackTag,1,4,ofp);
  FWRITE(&bytes_in,4,1,ofp);
  FWRITE(&bytes_out,4,1,ofp);
  FWRITE(&checksum,4,1,ofp);

  /* get size of input file */

  fseek(ifp,0,SEEK_END);
  inbytesremaining=ftell(ifp);
  fseek(ifp,0,SEEK_SET);

  /* encode the file, ifp -> ofp */

  encode(ifp,ofp);

  /* now we can update the total and checksum fields in the file header */

  fflush(ofp);
  fseek(ofp,4,SEEK_SET);
  FWRITE(&bytes_in,4,1,ofp);
  FWRITE(&bytes_out,4,1,ofp);
  FWRITE(&checksum,4,1,ofp);

  /* report totals */

  printf("%ld ==> %ld\n",bytes_in,bytes_out);

  /* finished */

  fclose(ifp);
  fclose(ofp);
  return(0);

  }


/*** END KLN ***/

