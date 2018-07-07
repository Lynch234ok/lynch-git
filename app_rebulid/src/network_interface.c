#include <hi_type.h>
#include <sdk/sdk_sys.h>
#include "network_interface.h"

#define MDIO_RWCTRL_ADDR				(0x10091100)
#define MDIO_RWCTRL_VALUE_ONE			(0x143)
#define MDIO_RWCTRL_VALUE_TWO			(0x145)
#define MDIO_RO_DATA_ADDR				(0x10091104)
#define NO_PHY_CONNECT					(0xffff)

/* ����:true���ߣ�false���� */
bool network_check_interface()
{
    unsigned phy_val1, phy_val2;
    bool ret = true;
    if(sdk_sys) {
        sdk_sys->write_reg(MDIO_RWCTRL_ADDR, MDIO_RWCTRL_VALUE_ONE);
        sdk_sys->read_reg(MDIO_RO_DATA_ADDR, &phy_val1);

        sdk_sys->write_reg(MDIO_RWCTRL_ADDR, MDIO_RWCTRL_VALUE_TWO);
        sdk_sys->read_reg(MDIO_RO_DATA_ADDR, &phy_val2);

        if(NO_PHY_CONNECT == phy_val1
            || NO_PHY_CONNECT == phy_val2){

            printf("There is no phy device connect !!! \n");
            ret = false;
        }else{//��ȡ��PHY�豸���ͽ�������

            printf("Phy device is connected !!! \n");
            ret = true;
        }
    }
    return ret;
}

int network_ifconf_set_interface(const char if_name[IFNAMSIZ], ifconf_interface_t* ifr)
{
    bool inter;
    inter = network_check_interface();
    if(inter) {
        ifconf_set_interface(if_name, ifr);
    }
    else {
    }

    return 0;
}

