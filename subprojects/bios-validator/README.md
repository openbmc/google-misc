# BIOS Validator
The BIOS Validator project consists of a library `libcr51sign` and a CLI tool
`bios-validator`.

## Cr51 Image Signature Library

### Package `libcr51sign`

*   Status: **Ready**

Libcr51sign is a library to verify images signed in the Cr51 format which can be
shared between all systems requiring this functionality. Given an absolute start
and end offset the library would scan for and validate the signature on the
image descriptor, if the image validates, hashes the rest of the image to verify
its integrity. Because this library will be used across many varied platforms,
it does not assume the presence of any standard libraries or operating system
interfaces. In order to handle this, a struct containing function pointers that
implement each piece of platform-specific functionality will be passed to the
library’s functions. Interface struct should typically be static data (could put
in rodata) while the data in context is mutable.

### Debug

Print will be handled via Macros. The user can define USER_PRINT or the library
would use its default. The library will not assert on any error conditions,but
will return error codes and expects the client to handle as deemed fit.

```

#ifndef USER_PRINT
#define CPRINTS(ctx, format, args...)printf(format, ##args)
#endif
```

### Prod/Dev transitions

Prod --> Prod: Allowed \
Prod --> Dev: Only if allowlisted/prod_to_dev_downgrade_allowed \
Dev --> Prod: Allowed \
Dev --> Dev: Allowed

verify_signature: The implementation should check if the signature size passed
is same as the length of the key

Note: libcr51sign will also provide a companion library with default SW
implementations of common functions like hash_init/final,
read_and_hash_update().

## BIOS Validator

A command line tool to verify BIOS images. Refer to the manual of the tool.

```
$ ./bios_validator validate --help
validate the embedded image descriptor in `BIOS_FILE` using the public key KEY_FILE.
Usage: ./build/bios_validator validate [OPTIONS] bios_file bios_size key_file

Positionals:
  bios_file TEXT:FILE REQUIRED
                              BIOS file to be validated
  bios_size UINT:UINT in [1 - 4294967295] REQUIRED
                              Size of the BIOS file
  key_file TEXT:FILE REQUIRED Public key in RSA4096_PKCS15 format

Options:
  -h,--help                   Print this help message and exit
  --write_version TEXT        File where the image version should be written

```
