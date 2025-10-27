#ifndef DEPENDS_H
#define DEPENDS_H

#define MAX_ARR_DEPENDS (2)
void wd_apply_depends(const char *depends_name);
void wd_install_depends(const char *dep_one, const char *dep_two);

#endif