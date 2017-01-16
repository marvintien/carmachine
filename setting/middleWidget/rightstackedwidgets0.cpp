#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
//#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
//#include <poll.h>
#include <stdbool.h>
#include "rightstackedwidgets0.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QThread>
#include "netconfigdialog.h"
#include "global_value.h"

#define DBG false

#if DBG
#define DEBUG_INFO(M, ...) qDebug("DEBUG %d: " M, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_INFO(M, ...) do {} while (0)
#endif

#define DEBUG_ERR(M, ...) qDebug("DEBUG %d: " M, __LINE__, ##__VA_ARGS__)

bool	lanOldState = false;
bool	lanNewState = false;

static const char WPA_SUPPLICANT_CONF_DIR[]           = "/tmp/wpa_supplicant.conf";
static const char HOSTAPD_CONF_DIR[]	=	"/tmp/hostapd.conf";
int is_supplicant_running();

// about wifi
int creat_supplicant_file()
{
    FILE* fp;
    fp = fopen(WPA_SUPPLICANT_CONF_DIR, "wt+");

    if (fp != 0) {
        fputs("ctrl_interface=/var/run/wpa_supplicant\n", fp);
        fputs("ap_scan=1\n", fp);
        fclose(fp);
        return 0;
    }
    return -1;
}

int creat_hostapd_file(const char* name, const char* password) {
    FILE* fp;
    fp = fopen(HOSTAPD_CONF_DIR, "wt+");

    if (fp != 0) {
        fputs("interface=wlan0\n", fp);
        fputs("driver=nl80211\n", fp);
        fputs("ssid=", fp);
        fputs(name, fp);
        fputs("\n", fp);
        fputs("channel=6\n", fp);
        fputs("hw_mode=g\n", fp);
        fputs("ieee80211n=1\n", fp);
        fputs("ht_capab=[SHORT-GI-20]\n", fp);
        fputs("ignore_broadcast_ssid=0\n", fp);
        fputs("auth_algs=1\n", fp);
        fputs("wpa=3\n", fp);
        fputs("wpa_passphrase=", fp);
        fputs(password, fp);
        fputs("\n", fp);
        fputs("wpa_key_mgmt=WPA-PSK\n", fp);
        fputs("wpa_pairwise=TKIP\n", fp);
        fputs("rsn_pairwise=CCMP", fp);

        fclose(fp);
        return 0;
    }
    return -1;
}


rightStackedWidgets0::rightStackedWidgets0(QWidget *parent):baseWidget(parent)
{
    setObjectName("rightStackedWidgets0");
    setStyleSheet("#rightStackedWidgets0{background:rgb(33,36,43)}");
    initLayout();
    initData();
    initConnection();  // 连接信号与槽、
}

void rightStackedWidgets0::initData()
{
    wifiWid = this;
    m_netManager = wpaManager::getInstance(this);
    m_wifiSwitch->getSwitchButton()->setToggle(is_supplicant_running());
    if(is_supplicant_running())
    {
        m_netManager->openCtrlConnection("wlan0");
    }
}

void rightStackedWidgets0::initLayout()
{
    // 第1行布局 开关
    m_wifiSwitch = new switchWidget(this);


    // 第2部分布局 TabWidget 包含2部分内容:Current status、WifiList
    m_tab = new QTabWidget(this);
    m_tab->setFont(QFont(Font_Family,Font_size_Normal+1,QFont::Normal));
    m_tab->setStyleSheet("background:rgb(33,36,43)");

    m_tabCurrentStatus = new tabCurrentStatus(this);
    m_tabScanResult = new tabScanResult(this);
    //    m_tabHotspot = new tabApHotspot(this);
    m_tab->addTab(m_tabScanResult,QString("Scan Result"));
    m_tab->addTab(m_tabCurrentStatus,QString("Current Status"));
    //    m_tab->addTab(m_tabHotspot,QString("HotSpot"));
    m_tab->setCurrentIndex(0);


    QVBoxLayout *vmainlyout = new QVBoxLayout;
    vmainlyout->addSpacing(30);
    vmainlyout->addWidget(m_wifiSwitch);
    vmainlyout->addSpacing(20);
    vmainlyout->addWidget(m_tab);
    vmainlyout->addStretch(0);
    vmainlyout->setContentsMargins(0,0,0,0);
    vmainlyout->setSpacing(12);

    // 空间太大显示不便，加一个横向的布局
    QHBoxLayout *hmainlyout = new QHBoxLayout;
    hmainlyout->addStretch(1);
    hmainlyout->addLayout(vmainlyout,4);
    hmainlyout->addStretch(1);
    setLayout(hmainlyout);
}

void rightStackedWidgets0::initConnection()
{
    if (access(WPA_SUPPLICANT_CONF_DIR, F_OK) < 0) {
        creat_supplicant_file();
    }
    creat_hostapd_file("RK_HOSTAPD_TEST", "12345678");

    //    connect(m_adapterSeletor,SIGNAL(activated(const QString&)),m_netManager,SLOT(selectAdapter(const QString&)));

    connect(m_tabCurrentStatus->connectButton,SIGNAL(clicked(bool)),m_netManager,SLOT(connectB()));
    connect(m_tabCurrentStatus->disconnectButton,SIGNAL(clicked(bool)),m_netManager,SLOT(disconnectB()));
    connect(m_tabScanResult->scanButton,SIGNAL(clicked(bool)),m_netManager,SLOT(scan()));

    connect(m_tabScanResult->m_table,SIGNAL(cellClicked(int,int)),this,SLOT(slot_showItemDetail(int,int)));
    connect(m_wifiSwitch->getSwitchButton(),SIGNAL(toggled(bool)),this,SLOT(slot_onToggled(bool)));

    m_workTimer = new QTimer(this);
    m_workTimer->setSingleShot(false);
    connect(m_workTimer, SIGNAL(timeout()), this, SLOT(slot_checkLanConnection()));
    m_workTimer->start(5000);
}

void rightStackedWidgets0::slot_showItemDetail(int row,int)
{
    netConfigDialog *dialog = new netConfigDialog(this);
    if (dialog == NULL)
        return;
    dialog->paramsFromScanResults(m_tabScanResult->m_netWorks[row]);
    dialog->show();
    dialog->exec();
}

const bool console_run(const char *cmdline) {
    DEBUG_INFO("cmdline = %s\n",cmdline);

#if 0
    FILE *fp = popen(cmdline, "r");
    if (!fp) {
        DEBUG_ERR("Running cmdline failed: %s\n", cmdline);
        return false;
    }

    pclose(fp);
#else
    int ret;
    ret = system(cmdline);
    if(ret < 0){
        DEBUG_ERR("Running cmdline failed: %s\n", cmdline);
    }
#endif

    return true;
}

int get_pid(char *Name) {
    int len;
    char name[20] = {0};
    len = strlen(Name);
    strncpy(name,Name,len);
    name[len] ='\0';
    char cmdresult[256] = {0};
    char cmd[20] = {0};
    FILE *pFile = NULL;
    int  pid = 0;

    sprintf(cmd, "pidof %s", name);
    pFile = popen(cmd, "r");
    if (pFile != NULL)  {
        while (fgets(cmdresult, sizeof(cmdresult), pFile)) {
            pid = atoi(cmdresult);
            DEBUG_INFO("--- %s pid = %d ---\n",name,pid);
            break;
        }
    }
    pclose(pFile);
    return pid;
}

int is_supplicant_running()
{
    int ret;

    ret = get_pid("wpa_supplicant");

    return ret;
}

int wifi_start_supplicant()
{
    if (is_supplicant_running()) {
        return 0;
    }

    console_run("/usr/sbin/wpa_supplicant -Dnl80211 -iwlan0 -c /tmp/wpa_supplicant.conf -B");

    return 0;
}

int wifi_stop_supplicant()
{
    //    int pid;
    //    char *cmd = NULL;

    //    /* Check whether supplicant already stopped */
    //    if (!is_supplicant_running()) {
    //        return 0;
    //   }

    //    pid = get_pid("wpa_supplicant");
    //    asprintf(&cmd, "kill %d", pid);
    //    console_run(cmd);
    //    free(cmd);

    //        return 0;

}

int is_hostapd_running()
{
    int ret;

    ret = get_pid("hostapd");

    return ret;
}

int wifi_start_hostapd()
{
    if (is_hostapd_running()) {
        return 0;
    }
    console_run("ifconfig wlan0 up");
    console_run("ifconfig wlan0 192.168.100.1 netmask 255.255.255.0");
    console_run("echo 1 > /proc/sys/net/ipv4/ip_forward");
    console_run("iptables --flush");
    console_run("iptables --table nat --flush");
    console_run("iptables --delete-chain");
    console_run("iptables --table nat --delete-chain");
    console_run("iptables --table nat --append POSTROUTING --out-interface eth0 -j MASQUERADE");
    console_run("iptables --append FORWARD --in-interface wlan0 -j ACCEPT");
    console_run("/usr/sbin/hostapd /tmp/hostapd.conf -B");

    return 0;
}

int wifi_stop_hostapd()
{
    //    int pid;
    //    char *cmd = NULL;

    //    if (!is_hostapd_running()) {
    //            return 0;
    //    }
    //    pid = get_pid("hostapd");
    //    asprintf(&cmd, "kill %d", pid);
    //    console_run(cmd);
    //    free(cmd);

    //    console_run("echo 0 > /proc/sys/net/ipv4/ip_forward");
    //    console_run("ifconfig wlan0 down");
    return 0;
}
void rightStackedWidgets0::wifiStationOn()
{

    myNetThread *thread = new myNetThread;
    thread->start();
    m_netManager->openCtrlConnection("wlan0");
}

void rightStackedWidgets0::wifiStationOff()
{
    wifi_stop_supplicant();
    m_tabScanResult->clearTable();
}

void myNetThread::run()
{
    wifi_start_supplicant();
}

void rightStackedWidgets0::wifiAccessPointOn()
{
    wifi_start_hostapd();
}

void rightStackedWidgets0::wifiAccessPointOff()
{
    wifi_stop_hostapd();
}
void rightStackedWidgets0::slot_onToggled(bool isChecked)
{
    if(isChecked){
        DEBUG_INFO("=======wifiStationOn========\n");
        wifiStationOn();
    }else{
        DEBUG_INFO("=======wifiStationOff========\n");
        wifiStationOff();
    }

}

void lanStateChanhe(bool state){
    //need to check wifi state
    if(state){
        console_run("ifconfig eth0 up");
        console_run("udhcpc -i eth0");
    }else{
        console_run("ifconfig eth0 down");
    }

}
void rightStackedWidgets0::slot_checkLanConnection()
{
    char cmdbuf[1024] = {0};
    char cmdresult[1024] = {0};

    sprintf(cmdbuf, "cat /sys/class/net/eth0/carrier");
    FILE *pp = popen(cmdbuf, "r");
    if (!pp) {
        DEBUG_ERR("Running cmdline failed:cat /sys/class/net/eth0/carrier\n");
        return;
    }
    fgets(cmdresult, sizeof(cmdresult), pp);
    pclose(pp);

    if(strstr(cmdresult, "1"))
    {
        lanNewState = true;
    }else if(strstr(cmdresult, "0")){
        lanNewState = false;
    }else{
        console_run("ifconfig eth0 up");
    }
    if(lanOldState != lanNewState){
        if(lanNewState){
            //LanConnected
            lanStateChanhe(lanNewState);
        }else{
            //LanDisconnected
            lanStateChanhe(lanNewState);
        }
        lanOldState = 	lanNewState;
    }

}
switchWidget::switchWidget(QWidget *parent):baseWidget(parent)
{
    QHBoxLayout *mainlyout = new QHBoxLayout;

    m_lblState = new QLabel(this);
    m_lblState->setFont(QFont(Font_Family,Font_size_Normal+1,QFont::Normal));
    m_lblState->setText("Open Wlan");

    m_btnSwitch = new switchButton(this);
    m_btnSwitch->setFixedSize(60,25);

    mainlyout->addWidget(m_lblState);
    mainlyout->addWidget(m_btnSwitch);
    mainlyout->setContentsMargins(0,0,0,0);
    mainlyout->setSpacing(0);

    setLayout(mainlyout);
}


// tabWidget中current status布局的初始化
tabCurrentStatus::tabCurrentStatus(QWidget *parent):baseWidget(parent)
{
    QVBoxLayout *vmainlyout = new QVBoxLayout;

    // lyout1_Status
    QHBoxLayout *lyout1 = new QHBoxLayout;
    QLabel *statusLabel = new QLabel(this);
    statusLabel->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    statusLabel->setText("Status:");

    textStatus = new QLabel(this);
    textStatus->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    textStatus->setText(QString("connected"));
    lyout1->addWidget(statusLabel,1);
    lyout1->addWidget(textStatus,2);

    //lyout2_Last Message
    QHBoxLayout *lyout2 = new QHBoxLayout;
    QLabel *lastMessageLabel = new QLabel(this);
    lastMessageLabel->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    lastMessageLabel->setText("Last message:");

    textLastMsg = new QLabel(this);
    textLastMsg->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    textLastMsg->setText(QString());
    lyout2->addWidget(lastMessageLabel,1);
    lyout2->addWidget(textLastMsg,2);

    // lyou3_Authenticant
    QHBoxLayout *lyout3 = new QHBoxLayout;
    QLabel *authenticationLabel = new QLabel(this);
    authenticationLabel->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    authenticationLabel->setText("Authentication:");

    textAuthentication = new QLabel(this);
    textAuthentication->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    textAuthentication->setText(QString("None"));
    lyout3->addWidget(authenticationLabel,1);
    lyout3->addWidget(textAuthentication,2);

    //lyout4_Encryption
    QHBoxLayout *lyout4 = new QHBoxLayout;
    QLabel *encryptionLabel = new QLabel(this);
    encryptionLabel->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    encryptionLabel->setText("EncryptionLabel:");

    textEncryption = new QLabel(this);
    textEncryption->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    textEncryption->setText(QString("None"));
    lyout4->addWidget(encryptionLabel,1);
    lyout4->addWidget(textEncryption,2);

    //lyout5_SSID
    QHBoxLayout *lyout5 = new QHBoxLayout;
    QLabel *ssidLabel = new QLabel(this);
    ssidLabel->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    ssidLabel->setText("SSID:");

    textSSID = new QLabel(this);
    textSSID->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    textSSID->setText(QString("None"));
    lyout5->addWidget(ssidLabel,1);
    lyout5->addWidget(textSSID,2);


    //lyout6_BSSID
    QHBoxLayout *lyout6 = new QHBoxLayout;
    QLabel *bssidLabel = new QLabel(this);
    bssidLabel->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    bssidLabel->setText("BSSID:");

    textBSSID = new QLabel(this);
    textBSSID->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    textBSSID->setText(QString("None"));
    lyout6->addWidget(bssidLabel,1);
    lyout6->addWidget(textBSSID,2);

    //lyout7_IP address
    QHBoxLayout *lyout7 = new QHBoxLayout;
    QLabel *ipAddressLabel = new QLabel(this);
    ipAddressLabel->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    ipAddressLabel->setText("IP Address:");

    textIPAddress = new QLabel(this);
    textIPAddress->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    textIPAddress->setText(QString("None"));
    lyout7->addWidget(ipAddressLabel,1);
    lyout7->addWidget(textIPAddress,2);

    //lyout8 connect and disconnect button
    QHBoxLayout *lyout8 = new QHBoxLayout;
    connectButton = new QPushButton("Connect",this);
    connectButton->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    connectButton->setStyleSheet("QPushButton{background:rgb(85,92,108)}"
                                 "QPushButton{color:white}"
                                 "QPushButton{border-radius:5px}");
    connectButton->setFixedSize(140,45);

    disconnectButton = new QPushButton("Disconnect",this);
    disconnectButton->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    disconnectButton->setStyleSheet("QPushButton{background:rgb(85,92,108)}"
                                    "QPushButton{color:white}"
                                    "QPushButton{border-radius:5px}");
    disconnectButton->setFixedSize(140,45);
    lyout8->addStretch(0);
    lyout8->addWidget(connectButton);
    lyout8->addSpacing(30);
    lyout8->addWidget(disconnectButton);
    lyout8->addStretch(0);

    vmainlyout->addLayout(lyout1);
    vmainlyout->addLayout(lyout2);
    vmainlyout->addLayout(lyout3);
    vmainlyout->addLayout(lyout4);
    vmainlyout->addLayout(lyout5);
    vmainlyout->addLayout(lyout6);
    vmainlyout->addLayout(lyout7);
    vmainlyout->addSpacing(40);
    vmainlyout->addLayout(lyout8);
    vmainlyout->addStretch(0);
    vmainlyout->setContentsMargins(30,30,10,10);

    setLayout(vmainlyout);
}

// tabWigdet中Scan Result布局的初始化
tabScanResult::tabScanResult(QWidget *parent):baseWidget(parent)
{
    QVBoxLayout *vmainlyout = new QVBoxLayout;

    // lyout1
    QLabel *tipLabel = new QLabel("the scan result:",this);
    tipLabel->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));

    // lyout2
    m_table = new wlanListTable(this);

    // lyout3
    QHBoxLayout *lyout1 = new QHBoxLayout;
    scanButton = new QPushButton("reScan",this);
    scanButton->setStyleSheet("QPushButton{background:rgb(85,92,108)}"
                              "QPushButton{color:white}"
                              "QPushButton{border-radius:5px}");
    scanButton->setFont(QFont(Font_Family,Font_size_Normal,QFont::Normal));
    scanButton->setFixedSize(120,45);

    lyout1->addStretch(0);
    lyout1->addWidget(scanButton);
    lyout1->addStretch(0);

    vmainlyout->addWidget(tipLabel);
    vmainlyout->addSpacing(5);
    vmainlyout->addWidget(m_table);
    vmainlyout->addSpacing(15);
    vmainlyout->addLayout(lyout1);
    vmainlyout->setContentsMargins(40,40,20,20);
    vmainlyout->setSpacing(5);
    setLayout(vmainlyout);
}

void tabScanResult::clearTable()
{
    int iLen = m_table->rowCount();
    for(int i=0;i<iLen;i++)
    {
        m_table->removeRow(0);
    }
    m_table->clear();
}

void tabScanResult::insertIntoTable(QString ssid, QString bssid, QString siganl, QString flags)
{
    int rowCount = m_table->rowCount();
    m_table->insertRow(rowCount);
    // 设置Wifi等级图标
    int siganlValue = siganl.toInt();
    QLabel *label = new QLabel("");
    if(siganlValue>=(-55)){
        label->setPixmap(QPixmap(":/image/setting/ic_wifi_signal_4_dark.png").scaled(40,40));
    }else if(siganlValue>=(-70)){
        label->setPixmap(QPixmap(":/image/setting/ic_wifi_signal_3_dark.png").scaled(40,40));
    }else if(siganlValue>=(-85)){
        label->setPixmap(QPixmap(":/image/setting/ic_wifi_signal_2_dark.png").scaled(40,40));
    }else{
        label->setPixmap(QPixmap(":/image/setting/ic_wifi_signal_1_dark.png").scaled(40,40));
    }
    QTableWidgetItem *siganlItem = new QTableWidgetItem();
    siganlItem->setData(Qt::DisplayRole,siganlValue); // 排序
    m_table->setItem(rowCount,2,siganlItem);
    m_table->setCellWidget(rowCount,2,label);


    // 设置安全性图标
    QString auth;
    if (flags.indexOf("[WPA2-EAP") >= 0)
        auth = "WPA2_EAP";
    else if (flags.indexOf("[WPA-EAP") >= 0)
        auth = "WPA_EAP";
    else if (flags.indexOf("[WPA2-PSK") >= 0)
        auth = "WPA2_PSK";
    else if (flags.indexOf("[WPA-PSK") >= 0)
        auth = "WPA_PSK";
    else
        auth = "OPEN";

    if(auth.compare("OPEN")==0)
    {
        m_table->setItem(rowCount,3,new QTableWidgetItem(QIcon(":/image/setting/ic_wifi_unlocked.png"),auth));
    }else{
        m_table->setItem(rowCount,3,new QTableWidgetItem(QIcon(":/image/setting/ic_wifi_locked.png"),auth));
    }

    m_table->setItem(rowCount,0,new QTableWidgetItem(ssid));
    m_table->setItem(rowCount,1,new QTableWidgetItem(bssid));
    m_table->item(rowCount,0)->setTextAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    m_table->item(rowCount,1)->setTextAlignment(Qt::AlignVCenter|Qt::AlignLeft);
    // 数据存入结构体数组中
    m_netWorks[rowCount].SSID = ssid;
    m_netWorks[rowCount].BSSID = bssid;
    m_netWorks[rowCount].signal = siganl;
    m_netWorks[rowCount].flags = flags;

    m_table->sortByColumn(2);
}

//tabApHotspot::tabApHotspot(QWidget *parent):baseWidget(parent)
//{
//    QVBoxLayout *vmainlyout = new QVBoxLayout;

//    // layout1 hotspot button
//    QHBoxLayout *lyout1 = new QHBoxLayout;
//    QLabel *switchText = new QLabel("Hotspot Switch:",this);
//    switchText->setFont(QFont("Microsoft YaHei",12,QFont::Normal));

//    m_hotspotSwitch = new switchButton(this);
//    m_hotspotSwitch->setFixedSize(40,20);
//    lyout1->addWidget(switchText);
//    lyout1->addWidget(m_hotspotSwitch);

//    //layout2 hotspot name edit
//    QHBoxLayout *lyout2 = new QHBoxLayout;
//    QLabel *hotspotNameText = new QLabel("Hotspot Name:",this);
//    hotspotNameText->setFont(QFont("Microsoft YaHei",12,QFont::Normal));

//    m_hotspotNameEdit = new QLineEdit(this);
//    m_hotspotNameEdit->setText("hotspot Name");

//    lyout2->addWidget(hotspotNameText);
//    lyout2->addWidget(m_hotspotNameEdit);

//    //layout3 confirm button
//    m_confirmButton = new QPushButton("confirm",this);
//    m_confirmButton->setStyleSheet("QPushButton{background:rgb(85,92,108)}"
//                              "QPushButton{color:white}"
//                              "QPushButton{border-radius:5px}");
//    m_confirmButton->setFixedSize(70,28);

//    vmainlyout->addSpacing(30);
//    vmainlyout->addLayout(lyout1);
//    vmainlyout->addLayout(lyout2);
//    vmainlyout->addSpacing(20);
//    vmainlyout->addWidget(m_confirmButton);
//    vmainlyout->addStretch(0);

//    vmainlyout->setContentsMargins(20,10,20,20);
//    vmainlyout->setSpacing(10);
//    setLayout(vmainlyout);
//}

