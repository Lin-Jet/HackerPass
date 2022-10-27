#include "include/algos.h"
#include "include/api.h"
#include "include/led.h"
#include "include/networking.h"
#include "include/rc522.h"
#include "config.h"

/* HTTP response code and response data buffers. */
uint16_t code;
String response;

/* Status variables. */
uint32_t eventID;
uint32_t id;
bool provisioning;

/* Buffers. */
byte buffer[12];
byte len;

/* Cached eventIDs and userIDs retrieved from the web. These are expected to be
   sorted web-side to offload computational cost as these will be b-searched. */
uint32_t eventIDs[MAX_EVENTS];
uint16_t lenEventIDs;
uint32_t userIDs[MAX_USERS];
uint16_t lenUserIDs;

void setup()
{
	Serial.begin(9600);
	rc522_init();
	led_esp_off();
	led_init();
	led_rgb_test();
	led_rgb_red();

	/* Initialize variables. */
	eventID = 0;  // Edge case when actual card ID = 0 (p = 2e-10). p is
	id = 0;       // sufficiently small to justify this micro-optimization.
	provisioning = false;

	/* Force a network connection to start. */
	while (!connect(SSID, PASSWORD, TIMEOUT));
	led_esp_on();

	/* TODO: populate eventIDs and userIDs from the API. */
	eventIDs[0] = 0x1479BC1F;  // Debugging value from prototype card.
	lenEventIDs = 1;

	/* Wait for an organizer to tap an event association card. This phase lasts
	   for a maximum time of 25.5 s, but will loop until successful. */
	while (true)
	{
		if (!read_uid(255, buffer, &len))  continue;
		id = (buffer[0] << 0x18) | (buffer[1] << 0x10) | (buffer[2] << 0x08) | buffer[3];

		if (bsearch_id(id, eventIDs, lenEventIDs) < 0)
		{
			led_rgb_off();
			delay(500);
			led_rgb_red();
		}
		else
		{
			led_rgb_white();
			break;
		}
	}
}

void loop()
{
	/* Check for a network connection. Attempt to reconnect if disconnected. */
	if (!is_connected())
	{
		led_esp_off();
		Serial.println("Not connected. MAC: " + get_mac());

		if (connect(SSID, PASSWORD, TIMEOUT))  led_esp_on();
		else                                   led_esp_off();
	}
	else  led_esp_on();

	/* Make sure the LED colors reflect the current mode. */
	if (provisioning)  led_rgb_purple();
	else               led_rgb_white();

	/* Try to provision a card. This will immediately call to the web. */
	if (provisioning)
	{
		/* TODO */
	}
	/* Try to read a card. This phase lasts for a maximum time of 10 s. */
	else if (read_uid(100, buffer, &len))
	{
		id = (buffer[0] << 0x18) | (buffer[1] << 0x10) | (buffer[2] << 0x08) | buffer[3];

		Serial.println("Card detected.");
		Serial.println(id);

		/* At this time, it is assumed that all cards are user cards. Provision
		   cards and event cards are yet to be implemented. */
		post(API, "/api/hp/attendance/create", HP_KEY, &code, &response,
			"{\"user\":"+String(id)+",\"event\":"+String(eventID)+"}");
		Serial.println(code);
		Serial.println(response);
	}
}
