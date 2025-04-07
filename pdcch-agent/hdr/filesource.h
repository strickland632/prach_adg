#ifndef SRSRAN_FILESOURCE_H
#define SRSRAN_FILESOURCE_H

#include <stdint.h>
#include <stdio.h>

#include "srsran/config.h"
#include "srsran/phy/io/format.h"

/* Low-level API */
typedef struct SRSRAN_API {
  FILE *f;
  srsran_datatype_t type;
} srsran_filesource_t;

SRSRAN_API int srsran_filesource_init(srsran_filesource_t *q,
                                      const char *filename,
                                      srsran_datatype_t type);

SRSRAN_API void srsran_filesource_free(srsran_filesource_t *q);

SRSRAN_API void srsran_filesource_seek(srsran_filesource_t *q, int pos);

SRSRAN_API int srsran_filesource_read(srsran_filesource_t *q, void *buffer,
                                      int nsamples);

SRSRAN_API int srsran_filesource_read_multi(srsran_filesource_t *q,
                                            void **buffer, int nsamples,
                                            int nof_channels);

#endif // SRSRAN_FILESOURCE_H
