#include <QVBoxLayout>
#include <QProgressBar>
#include <QDesktopServices>
#include "updates_check_widget.h"
#include "updates_checker.h"
#include "version.h"

#define CHECKING_WIDGET_INDEX           0
#define SHOWING_RESULT_WIDGET_INDEX     1

UpdatesCheckWidget::UpdatesCheckWidget()
{
    setWindowTitle(applicationTitle(tr("Check for Updates")));
    
    m_stackedWidget = new QStackedWidget;
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_stackedWidget);
    
    QWidget *checkingWidget = new QWidget;
    QWidget *showingResultWidget = new QWidget;
    
    ////////// checking ///////////////////
    
    QLabel *busyLabel = new QLabel;
    busyLabel->setText(tr("Checking for updates..."));
    
    QProgressBar *busyBar = new QProgressBar;
    busyBar->setMaximum(0);
    busyBar->setMinimum(0);
    busyBar->setValue(0);
    
    QVBoxLayout *checkingLayout = new QVBoxLayout;
    checkingLayout->addWidget(busyLabel);
    checkingLayout->addWidget(busyBar);
    
    checkingWidget->setLayout(checkingLayout);
    
    ////////// showing result /////////////
    
    m_infoLabel = new QLabel;
    
    m_viewButton = new QPushButton(tr("View"));
    m_viewButton->hide();
    
    connect(m_viewButton, &QPushButton::clicked, this, &UpdatesCheckWidget::viewUpdates);
    
    QVBoxLayout *showingResultLayout = new QVBoxLayout;
    showingResultLayout->addWidget(m_infoLabel);
    showingResultLayout->addStretch();
    showingResultLayout->addWidget(m_viewButton);
    
    showingResultWidget->setLayout(showingResultLayout);
    
    m_stackedWidget->addWidget(checkingWidget);
    m_stackedWidget->addWidget(showingResultWidget);
    
    m_stackedWidget->setCurrentIndex(CHECKING_WIDGET_INDEX);
    
    setLayout(mainLayout);
}

UpdatesCheckWidget::~UpdatesCheckWidget()
{
    delete m_updatesChecker;
}

void UpdatesCheckWidget::viewUpdates()
{
    if (m_viewUrl.isEmpty())
        return;
    
    QDesktopServices::openUrl(QUrl(m_viewUrl));
}

void UpdatesCheckWidget::check()
{
    if (nullptr != m_updatesChecker)
        return;
    
    m_stackedWidget->setCurrentIndex(CHECKING_WIDGET_INDEX);
    
    m_viewUrl.clear();
    
    m_updatesChecker = new UpdatesChecker;
    connect(m_updatesChecker, &UpdatesChecker::finished, this, &UpdatesCheckWidget::checkFinished);
    m_updatesChecker->start();
}

void UpdatesCheckWidget::checkFinished()
{
    m_infoLabel->setText(m_updatesChecker->message());
    if (m_updatesChecker->hasError()) {
        m_viewButton->hide();
    } else {
        if (m_updatesChecker->isLatest()) {
            m_viewButton->hide();
        } else {
            m_viewUrl = m_updatesChecker->matchedUpdateItem().descriptionUrl;
            m_viewButton->show();
        }
    }
    m_stackedWidget->setCurrentIndex(SHOWING_RESULT_WIDGET_INDEX);
    
    m_updatesChecker->deleteLater();
    m_updatesChecker = nullptr;
}
