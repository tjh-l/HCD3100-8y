#ifndef __COM_LOGO_H__
#define __COM_LOGO_H__

int com_logo_show(const char *file_path, int rotate_type, int mirror_type);
void com_logo_off(int closevp, int fillblack);
int com_logo_dis_backup_free(void);
int com_logo_dis_backup(void);

#endif
