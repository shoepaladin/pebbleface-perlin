module.exports = [
  {
    "type": "heading",
    "defaultValue": "Perlin"
  },
  {
    "type": "text",
    "defaultValue": "Rotating perlin-noise backgrounds with time, date, battery and a daily steps bar."
  },

  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Display",
        "size": 3
      },
      {
        "type": "toggle",
        "messageKey": "showdate",
        "label": "Show date",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "showbatt",
        "label": "Show battery percentage",
        "defaultValue": true
      }
    ]
  },

  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Steps",
        "size": 3
      },
      {
        "type": "toggle",
        "messageKey": "showsteps",
        "label": "Show steps progress bar",
        "defaultValue": true
      },
      {
        "type": "input",
        "messageKey": "maxsteps",
        "label": "Daily step goal",
        "defaultValue": "10000",
        "attributes": {
          "type": "number",
          "min": 100,
          "max": 100000
        }
      }
    ]
  },

  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Vibration",
        "size": 3
      },
      {
        "type": "toggle",
        "messageKey": "bluetoothvibe",
        "label": "Vibrate on Bluetooth disconnect",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "hourlyvibe",
        "label": "Vibrate on the hour",
        "defaultValue": false
      }
    ]
  },

  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Background",
        "size": 3
      },
      {
        "type": "toggle",
        "messageKey": "randomtime",
        "label": "Shuffle background when saving",
        "defaultValue": false
      },
      {
        "type": "radiogroup",
        "messageKey": "refreshhours",
        "label": "Change background every",
        "defaultValue": "1",
        "options": [
          { "label": "1 hour",   "value": "1"  },
          { "label": "5 hours",  "value": "5"  },
          { "label": "15 hours", "value": "15" },
          { "label": "30 hours", "value": "30" },
          { "label": "60 hours", "value": "60" }
        ]
      }
    ]
  },

  {
    "type": "submit",
    "defaultValue": "Save"
  }
];
