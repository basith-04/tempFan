void __libc_init_array(void) {}

void *memset(void *destination, int value, unsigned long count)
{
  unsigned char *p = destination;
  while (count-- != 0) *p++ = (unsigned char)value;
  return destination;
}

void *memcpy(void *destination, const void *source, unsigned long count)
{
  unsigned char *d = destination;
  const unsigned char *s = source;
  while (count-- != 0) *d++ = *s++;
  return destination;
}
