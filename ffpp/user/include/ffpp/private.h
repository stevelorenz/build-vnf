/*
 * private.h
 */

#ifndef PRIVATE_H
#define PRIVATE_H

#define EXPORT __attribute__((visibility("default")))

#if defined(__clang__) || defined(__GNUC__)
#define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
#else
#define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

#endif /* !PRIVATE_H */
