/*
 * $Id: byteswap.c,v 1.2 2000/01/27 14:41:25 lees Exp $
 */

#include "basic.h"

void WriteByte(FILE *fp, byte c)
{
  fwrite(&c, sizeof(byte), 1, fp);
}

byte ReadByte(FILE *fp)
{
  byte c;
  fread(&c, sizeof(byte), 1, fp);
  return c;
}

short BigShort(short l)
{
    byte b1, b2;

    b1 = l & 255;
    b2 = (l >> 8) & 255;

    return (b1 << 8) + b2;
}

void WriteShort(FILE *fp, short l)
{
  short tmp;
  tmp = BigShort(l);
  fwrite(&tmp, sizeof(short), 1, fp);
}

short ReadShort(FILE *fp)
{
  short tmp;
  fread(&tmp, sizeof(short), 1, fp);
  return BigShort(tmp);
}

long BigLong(long l)
{
    byte b1, b2, b3, b4;

    b1 = l & 255;
    b2 = (l >> 8) & 255;
    b3 = (l >> 16) & 255;
    b4 = (l >> 24) & 255;

    return ((long)b1 << 24) + ((long)b2 << 16) + ((long)b3 << 8) + b4;
}

void WriteLong(FILE *fp, long l)
{
  long tmp;
  tmp = BigLong(l);
  fwrite(&tmp, sizeof(long), 1, fp);
}

long ReadLong(FILE *fp)
{
  long tmp;
  fread(&tmp, sizeof(long), 1, fp);
  return BigLong(tmp);
}

float BigFloat(float l)
{
    union {
	byte b[4];
	float f;
    } in, out;

    in.f = l;
    out.b[0] = in.b[3];
    out.b[1] = in.b[2];
    out.b[2] = in.b[1];
    out.b[3] = in.b[0];

    return out.f;
}
