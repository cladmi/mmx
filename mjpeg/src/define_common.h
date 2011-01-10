#ifndef DEFINE_COMMON_H
#define DEFINE_COMMON_H

#define SWITCH_ENDIAN_32(x)   \
        x = ((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8)  |  \
            ((x & 0x00ff0000) >> 8)  | ((x & 0xff000000) >> 24)

#define SWITCH_ENDIAN_16(x) \
       x = ((x & 0x00ff) << 8) | ((x & 0xff00) >> 8)
#define CPU_DATA_IS_BIGENDIAN(type,data) SWITCH_ENDIAN_##type(data)


#define NEXT_TOKEN(res) {                \
   fread ((void *) & res, 1, 1, movie);  \
}

#define COPY_SECTION(to, size) {        \
   fread ((void *)to, 1, size, movie);  \
}

#define SKIP(n) {                                                                                              \
   uint8_t waste[2048];                                                                                     \
                                                                                                                        \
   if (n != 0) fread ((void *) & waste, n, 1, movie);                   \
}


#ifdef VERBOSE
#define VPRINTF(format, ...) printf ("[%s] " format, __FUNCTION__, ## __VA_ARGS__)
#else
#define VPRINTF(format, ...)
#endif

#ifdef INFO
#define IPRINTF(format, ...) printf ("[%s] " format, __FUNCTION__, ## __VA_ARGS__)
#else
#define IPRINTF(format, ...)
#endif

#endif //DEFINE_COMMON_H


