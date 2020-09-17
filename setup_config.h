#ifndef SETUP_CONFIG_H
#define SETUP_CONFIG_H

void setupConfig(bool forceAp);
void loopConfig();

extern void resetToStartConfig();
extern void resetToEndConfig();

extern void blast(const char *bits);

#endif /* SETUP_CONFIG_H */
