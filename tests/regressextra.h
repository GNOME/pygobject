#ifndef REGRESS_EXTRA_H
#define REGRESS_EXTRA_H

typedef struct _RegressTestBoxedC RegressTestBoxedC;
typedef struct _RegressTestBoxedCWrapper RegressTestBoxedCWrapper;

_GI_TEST_EXTERN
GType regress_test_boxed_c_wrapper_get_type (void);

_GI_TEST_EXTERN
RegressTestBoxedCWrapper *regress_test_boxed_c_wrapper_new (void);
_GI_TEST_EXTERN
RegressTestBoxedCWrapper * regress_test_boxed_c_wrapper_copy (RegressTestBoxedCWrapper *self);
_GI_TEST_EXTERN
RegressTestBoxedC *regress_test_boxed_c_wrapper_get (RegressTestBoxedCWrapper *self);

#endif /* REGRESS_EXTRA_H */
