#ifndef  ML_EYEFI_TRICKS_H
#define  ML_EYEFI_TRICKS_H

#ifdef FEATURE_EYEFI_TRICKS
int check_eyefi();
void EyeFi_RenameCR2toAVI(char* dir);
void EyeFi_RenameAVItoCR2(char* dir);

#ifdef FEATURE_EYEFI_RENAME_422_MP4
void EyeFi_Rename422toMP4(char* dir);
void EyeFi_RenameMP4to422(char* dir);
#endif // FEATURE_EYEFI_RENAME_422_MP4

#endif // FEATURE_EYEFI_TRICKS

#endif // ML_EYEFI_TRICKS_H
