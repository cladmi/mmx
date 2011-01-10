#ifndef __SCREEN_H__
#define __SCREEN_H__

extern void screen_init(uint32_t width, uint32_t height);
extern int screen_exit();
extern void screen_memcpy(uint32_t x, uint32_t y, void *ptr, uint32_t nbpixels);
extern void screen_cpyrect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, void *ptr);
extern int screen_refresh() ;

#endif

