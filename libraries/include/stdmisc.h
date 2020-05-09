#ifndef _STDMISC_H_
#define	_STDMISC_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

extern int xatoi (char **str,      /* Pointer to pointer to the string */
                  long *res);      /* Pointer to the valiable to store the value */

extern int uxatoi(char **str,          /* Pointer to pointer to the string */
                  uint32_t *res);      /* Pointer to the valiable to store the value */

#ifdef __cplusplus
}
#endif

#endif /* _STDMISC_H_ */
