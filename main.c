#include "dcpvrdump.h"

#include <assert.h>

int main()
{
    assert(sizeof(struct PVD_header) == 20);
}