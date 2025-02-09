#ifndef DUST3D_APPLICATION_THEME_H_
#define DUST3D_APPLICATION_THEME_H_

#include <QColor>
#include <QChar>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include "QtAwesome.h"

class Theme
{
public:
    static QColor white;
    static QColor red;
    static QColor green;
    static QColor blue;
    static float normalAlpha;
    static float checkedAlpha;
    static float edgeAlpha;
    static float fillAlpha;
    static int toolIconFontSize;
    static int toolIconSize;
    static int miniIconFontSize;
    static int miniIconSize;
    static int partPreviewImageSize;
    static int materialPreviewImageSize;
    static int sidebarPreferredWidth;
    
    static void initialize();
    static QtAwesome *awesome();
    static void initAwesomeButton(QPushButton *button);
    static void initAwesomeLabel(QLabel *label);
    static void initAwesomeMiniButton(QPushButton *button);
    static void updateAwesomeMiniButton(QPushButton *button, QChar icon, bool highlighted, bool enabled, bool unnormal=false);
    static void initAwesomeToolButton(QPushButton *button);
    static void initAwesomeToolButtonWithoutFont(QPushButton *button);
    static void initAwsome();
    static void initToolButton(QPushButton *button);
    static void initCheckbox(QCheckBox *checkbox);
};

#endif
