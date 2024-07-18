#include <linux/module.h>
#include <linux/usb.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>

#define USB_ID_RME_BABYFACE_PRO 0x2a39, 0x3fb0

#define SND_BBFPRO_USBREQ_GAIN 0x1a
#define SND_BBFPRO_GAIN_CHANNELS 4
#define SND_BBFPRO_GAIN_MIN1 0
#define SND_BBFPRO_GAIN_MIN2 0
#define SND_BBFPRO_GAIN_MAX1 65
#define SND_BBFPRO_GAIN_MAX2 24

struct snd_bbfpro {
    struct usb_device *dev;
    struct snd_card *card;
    struct usb_interface *intf;
    struct list_head midi_list;
    int num_midis;
    struct list_head list;
};

static int snd_bbfpro_gain_update(struct usb_mixer_interface *mixer, u8 channel, int gain)
{
    struct snd_usb_audio *chip = mixer->chip;
    int err;
    u16 wValue;

    err = snd_usb_lock_shutdown(chip);
    if (err < 0)
        return err;

    // Adjust gain value based on channel
    if (channel < 2) {
        // For channels 1 and 2
        if (gain < 0)
            gain = 0;
        else if (gain > 65)
            gain = 65;
        // Handle the quirk for 0x01 - 0x09 gap
        if (gain > 0 && gain < 10)
            gain = 10;
    } else {
        // For channels 3 and 4
        if (gain < 0)
            gain = 0;
        else if (gain > 24)
            gain = 24;
    }

    wValue = gain;

    err = snd_usb_ctl_msg(chip->dev,
                          usb_sndctrlpipe(chip->dev, 0),
                          SND_BBFPRO_USBREQ_GAIN,  // Use the gain-specific request
                          USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
                          wValue, channel, NULL, 0);

    snd_usb_unlock_shutdown(chip);
    return err;
}

static int snd_bbfpro_gain_get(struct snd_kcontrol *kcontrol,
                               struct snd_ctl_elem_value *ucontrol)
{
    int channel = (kcontrol->private_value >> 16) & 0xff;
    int value = kcontrol->private_value & 0xffff;
    ucontrol->value.integer.value[0] = value;
    return 0;
}

static int snd_bbfpro_gain_info(struct snd_kcontrol *kcontrol,
                                struct snd_ctl_elem_info *uinfo)
{
    int channel = (kcontrol->private_value >> 16) & 0xff;

    uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
    uinfo->count = 1;
    uinfo->value.integer.min = 0;
    
    if (channel < 2) {
        uinfo->value.integer.max = 65;
    } else {
        uinfo->value.integer.max = 24;
    }
    
    return 0;
}

static int snd_bbfpro_gain_put(struct snd_kcontrol *kcontrol,
                               struct snd_ctl_elem_value *ucontrol)
{
    struct usb_mixer_elem_list *list = snd_kcontrol_chip(kcontrol);
    struct usb_mixer_interface *mixer = list->mixer;
    int channel = (kcontrol->private_value >> 16) & 0xff;
    int value = ucontrol->value.integer.value[0];
    int err;

    if (channel < 2) {
        if (value > 65)
            return -EINVAL;
        if (value > 0 && value < 10)
            value = 10;  // Handle the quirk for 0x01 - 0x09 gap
    } else {
        if (value > 24)
            return -EINVAL;
    }

    if (value == (kcontrol->private_value & 0xffff))
        return 0;

    err = snd_bbfpro_gain_update(mixer, channel, value);
    if (err < 0)
        return err;

    kcontrol->private_value = (channel << 16) | value;
    return 1;
}

static const struct snd_kcontrol_new snd_bbfpro_gain_control = {
    .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
    .name = "", // to be filled in later
    .info = snd_bbfpro_gain_info,
    .get = snd_bbfpro_gain_get,
    .put = snd_bbfpro_gain_put,
};

static int snd_bbfpro_gain_add(struct usb_mixer_interface *mixer, int channel, char *name)
{
    struct snd_kcontrol_new knew = snd_bbfpro_gain_control;
    knew.name = name;
    knew.private_value = channel << 16;
    return snd_usb_mixer_add_control(mixer, &knew, 0, NULL);
}

for (int i = 0; i < 4; i++) {
    char name[32];
    snprintf(name, sizeof(name), "Input %d Gain", i + 1);
    err = snd_bbfpro_gain_add(mixer, i, name);
    if (err < 0)
        return err;
}

// static int snd_bbfpro_gain_resume(struct usb_mixer_elem_list *list)
// {
//     struct usb_mixer_interface *mixer = list->mixer;
//     struct snd_kcontrol *kcontrol = list->kctl;
//     int channel = (kcontrol->private_value >> 16) & 0xff;
//     int value = kcontrol->private_value & 0xffff;

//     return snd_bbfpro_gain_update(mixer, channel, value);
// }

// static const struct snd_kcontrol_new snd_bbfpro_gain_control = {
//     .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
//     .name = "", // to be filled in later
//     .info = snd_bbfpro_gain_info,
//     .get = snd_bbfpro_gain_get,
//     .put = snd_bbfpro_gain_put,
// };

// static int snd_bbfpro_gain_add(struct usb_mixer_interface *mixer)
// {
//     char name[44];
//     int channel, err;

//     for (channel = 0; channel < SND_BBFPRO_GAIN_CHANNELS; channel++) {
//         sprintf(name, "Input %d Gain", channel + 1);
//         err = snd_usb_mixer_add_control(mixer, 
//                                         &snd_bbfpro_gain_control,
//                                         channel << 16,  // private_value
//                                         name,
//                                         snd_bbfpro_gain_resume);
//         if (err < 0)
//             return err;
//     }

//     return 0;
// }

// static int snd_bbfpro_create_mixer(struct snd_bbfpro *bbfpro, struct usb_interface *intf)
// {
//     struct usb_mixer_interface *mixer;
//     int err;

//     mixer = kzalloc(sizeof(*mixer), GFP_KERNEL);
//     if (!mixer)
//         return -ENOMEM;

//     mixer->chip = bbfpro;
//     mixer->id_elems = NULL;
//     mixer->usb_id = USB_ID(USB_ID_RME_BABYFACE_PRO);
//     mixer->intf = intf;

//     err = snd_bbfpro_gain_add(mixer);
//     if (err < 0) {
//         kfree(mixer);
//         return err;
//     }

//     usb_set_intfdata(intf, mixer);
//     return 0;
// }

// static int snd_bbfpro_probe(struct usb_interface *intf, const struct usb_device_id *id)
// {
//     struct snd_card *card;
//     struct snd_bbfpro *bbfpro;
//     int err;

//     err = snd_card_new(&intf->dev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1,
//                        THIS_MODULE, sizeof(*bbfpro), &card);
//     if (err < 0)
//         return err;

//     bbfpro = card->private_data;
//     bbfpro->dev = interface_to_usbdev(intf);
//     bbfpro->card = card;
//     bbfpro->intf = intf;

//     err = snd_bbfpro_create_mixer(bbfpro, intf);
//     if (err < 0)
//         goto error;

//     strcpy(card->driver, "RME Babyface Pro");
//     strcpy(card->shortname, "RME Babyface Pro");
//     sprintf(card->longname, "%s at %s", card->shortname,
//             dev_name(&bbfpro->dev->dev));

//     err = snd_card_register(card);
//     if (err < 0)
//         goto error;

//     usb_set_intfdata(intf, bbfpro);
//     return 0;

// error:
//     snd_card_free(card);
//     return err;
// }

// static void snd_bbfpro_disconnect(struct usb_interface *intf)
// {
//     struct snd_bbfpro *bbfpro = usb_get_intfdata(intf);

//     if (!bbfpro)
//         return;

//     snd_card_disconnect(bbfpro->card);
//     snd_card_free_when_closed(bbfpro->card);
// }

// static struct usb_device_id snd_bbfpro_ids[] = {
//     { USB_DEVICE(USB_ID_RME_BABYFACE_PRO) },
//     { }
// };
// MODULE_DEVICE_TABLE(usb, snd_bbfpro_ids);

// static struct usb_driver snd_bbfpro_driver = {
//     .name = "snd-usb-bbfpro",
//     .probe = snd_bbfpro_probe,
//     .disconnect = snd_bbfpro_disconnect,
//     .id_table = snd_bbfpro_ids,
// };

// module_usb_driver(snd_bbfpro_driver);

// MODULE_AUTHOR("Flumie but actually chatgpt");
// MODULE_DESCRIPTION("RME Babyface Pro Set Gain patch");
// MODULE_LICENSE("GPL");