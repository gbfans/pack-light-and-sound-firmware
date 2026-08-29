# Animation Gallery

These previews are rendered by the simulator in [`SOFTWARE/sim`](SOFTWARE/sim), which compiles the firmware's own animation sources (`animations.cpp`, `party_sequences.cpp`, the RAMP library) and drives them with the same play/stop sequencing the pack state machine uses - what the GIFs show is what the firmware does. Each pack-mode clip runs the complete power-up, idle, and shut-down sequence; the N-Filter column shows the vent light, which animates only during vent sequences (the vent relay output is held on at the same time for smoke effects). Each GIF links to a corresponding MP4 recording.

## Movie / Snap Mode (GB1/GB2)
DIP1 – OFF, DIP2 – OFF, DIP3 – OFF

| Cyclotron LEDs | Powercell (15) | Cyclotron | N-Filter (16, vent light) |
| --- | --- | --- | --- |
| 4 | ![](SOFTWARE/animations/movie_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/movie_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/movie_cyclotron_sequence_4_red.gif) [MP4](SOFTWARE/animations/movie_cyclotron_sequence_4_red.mp4) | ![](SOFTWARE/animations/movie_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/movie_future_sequence_16_white.mp4) |
| 24 | ![](SOFTWARE/animations/movie_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/movie_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/movie_cyclotron_sequence_24_red.gif) [MP4](SOFTWARE/animations/movie_cyclotron_sequence_24_red.mp4) | ![](SOFTWARE/animations/movie_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/movie_future_sequence_16_white.mp4) |
| 32 | ![](SOFTWARE/animations/movie_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/movie_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/movie_cyclotron_sequence_32_red.gif) [MP4](SOFTWARE/animations/movie_cyclotron_sequence_32_red.mp4) | ![](SOFTWARE/animations/movie_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/movie_future_sequence_16_white.mp4) |
| 40 | ![](SOFTWARE/animations/movie_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/movie_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/movie_cyclotron_sequence_40_red.gif) [MP4](SOFTWARE/animations/movie_cyclotron_sequence_40_red.mp4) | ![](SOFTWARE/animations/movie_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/movie_future_sequence_16_white.mp4) |

## Video Game / Fade Mode
DIP1 – ON, DIP2 – OFF, DIP3 – OFF

| Cyclotron LEDs | Powercell (15) | Cyclotron | N-Filter (16, vent light) |
| --- | --- | --- | --- |
| 4 | ![](SOFTWARE/animations/video_game_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/video_game_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/video_game_cyclotron_sequence_4_red.gif) [MP4](SOFTWARE/animations/video_game_cyclotron_sequence_4_red.mp4) | ![](SOFTWARE/animations/video_game_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/video_game_future_sequence_16_white.mp4) |
| 24 | ![](SOFTWARE/animations/video_game_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/video_game_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/video_game_cyclotron_sequence_24_red.gif) [MP4](SOFTWARE/animations/video_game_cyclotron_sequence_24_red.mp4) | ![](SOFTWARE/animations/video_game_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/video_game_future_sequence_16_white.mp4) |
| 32 | ![](SOFTWARE/animations/video_game_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/video_game_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/video_game_cyclotron_sequence_32_red.gif) [MP4](SOFTWARE/animations/video_game_cyclotron_sequence_32_red.mp4) | ![](SOFTWARE/animations/video_game_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/video_game_future_sequence_16_white.mp4) |
| 40 | ![](SOFTWARE/animations/video_game_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/video_game_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/video_game_cyclotron_sequence_40_red.gif) [MP4](SOFTWARE/animations/video_game_cyclotron_sequence_40_red.mp4) | ![](SOFTWARE/animations/video_game_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/video_game_future_sequence_16_white.mp4) |

## TVG Mode
DIP1 – OFF, DIP2 – ON, DIP3 – OFF

| Cyclotron LEDs | Powercell (15) | Cyclotron | N-Filter (16, vent light) |
| --- | --- | --- | --- |
| 4 | ![](SOFTWARE/animations/tvg_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/tvg_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/tvg_cyclotron_sequence_4_red.gif) [MP4](SOFTWARE/animations/tvg_cyclotron_sequence_4_red.mp4) | ![](SOFTWARE/animations/tvg_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/tvg_future_sequence_16_white.mp4) |
| 24 | ![](SOFTWARE/animations/tvg_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/tvg_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/tvg_cyclotron_sequence_24_red.gif) [MP4](SOFTWARE/animations/tvg_cyclotron_sequence_24_red.mp4) | ![](SOFTWARE/animations/tvg_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/tvg_future_sequence_16_white.mp4) |
| 32 | ![](SOFTWARE/animations/tvg_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/tvg_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/tvg_cyclotron_sequence_32_red.gif) [MP4](SOFTWARE/animations/tvg_cyclotron_sequence_32_red.mp4) | ![](SOFTWARE/animations/tvg_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/tvg_future_sequence_16_white.mp4) |
| 40 | ![](SOFTWARE/animations/tvg_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/tvg_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/tvg_cyclotron_sequence_40_red.gif) [MP4](SOFTWARE/animations/tvg_cyclotron_sequence_40_red.mp4) | ![](SOFTWARE/animations/tvg_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/tvg_future_sequence_16_white.mp4) |

## Afterlife Mode
DIP1 – ON, DIP2 – ON, DIP3 – OFF

| Cyclotron LEDs | Powercell (15) | Cyclotron | N-Filter (16, vent light) |
| --- | --- | --- | --- |
| 4 | ![](SOFTWARE/animations/afterlife_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/afterlife_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/afterlife_cyclotron_sequence_4_red.gif) [MP4](SOFTWARE/animations/afterlife_cyclotron_sequence_4_red.mp4) | ![](SOFTWARE/animations/afterlife_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/afterlife_future_sequence_16_white.mp4) |
| 24 | ![](SOFTWARE/animations/afterlife_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/afterlife_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/afterlife_cyclotron_sequence_24_red.gif) [MP4](SOFTWARE/animations/afterlife_cyclotron_sequence_24_red.mp4) | ![](SOFTWARE/animations/afterlife_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/afterlife_future_sequence_16_white.mp4) |
| 32 | ![](SOFTWARE/animations/afterlife_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/afterlife_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/afterlife_cyclotron_sequence_32_red.gif) [MP4](SOFTWARE/animations/afterlife_cyclotron_sequence_32_red.mp4) | ![](SOFTWARE/animations/afterlife_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/afterlife_future_sequence_16_white.mp4) |
| 40 | ![](SOFTWARE/animations/afterlife_powercell_sequence_15_blue.gif) [MP4](SOFTWARE/animations/afterlife_powercell_sequence_15_blue.mp4) | ![](SOFTWARE/animations/afterlife_cyclotron_sequence_40_red.gif) [MP4](SOFTWARE/animations/afterlife_cyclotron_sequence_40_red.mp4) | ![](SOFTWARE/animations/afterlife_future_sequence_16_white.gif) [MP4](SOFTWARE/animations/afterlife_future_sequence_16_white.mp4) |

## Party Mode
Activate by starting a song with the song switch, then tapping the fire button while the pack is off. All LEDs participate in the selected party pattern.

| Pattern | Powercell 15 | Future 16 | Cyclotron 4 | Cyclotron 24 | Cyclotron 32 | Cyclotron 40 |
| --- | --- | --- | --- | --- | --- | --- |
| Rainbow Fade | ![](SOFTWARE/animations/party_rainbow_fade_15_rainbow.gif) [MP4](SOFTWARE/animations/party_rainbow_fade_15_rainbow.mp4) | ![](SOFTWARE/animations/party_rainbow_fade_16_rainbow.gif) [MP4](SOFTWARE/animations/party_rainbow_fade_16_rainbow.mp4) | ![](SOFTWARE/animations/party_rainbow_fade_4_rainbow.gif) [MP4](SOFTWARE/animations/party_rainbow_fade_4_rainbow.mp4) | ![](SOFTWARE/animations/party_rainbow_fade_24_rainbow.gif) [MP4](SOFTWARE/animations/party_rainbow_fade_24_rainbow.mp4) | ![](SOFTWARE/animations/party_rainbow_fade_32_rainbow.gif) [MP4](SOFTWARE/animations/party_rainbow_fade_32_rainbow.mp4) | ![](SOFTWARE/animations/party_rainbow_fade_40_rainbow.gif) [MP4](SOFTWARE/animations/party_rainbow_fade_40_rainbow.mp4) |
| Cylon Scanner | ![](SOFTWARE/animations/party_cylon_scanner_15_rainbow.gif) [MP4](SOFTWARE/animations/party_cylon_scanner_15_rainbow.mp4) | ![](SOFTWARE/animations/party_cylon_scanner_16_rainbow.gif) [MP4](SOFTWARE/animations/party_cylon_scanner_16_rainbow.mp4) | ![](SOFTWARE/animations/party_cylon_scanner_4_rainbow.gif) [MP4](SOFTWARE/animations/party_cylon_scanner_4_rainbow.mp4) | ![](SOFTWARE/animations/party_cylon_scanner_24_rainbow.gif) [MP4](SOFTWARE/animations/party_cylon_scanner_24_rainbow.mp4) | ![](SOFTWARE/animations/party_cylon_scanner_32_rainbow.gif) [MP4](SOFTWARE/animations/party_cylon_scanner_32_rainbow.mp4) | ![](SOFTWARE/animations/party_cylon_scanner_40_rainbow.gif) [MP4](SOFTWARE/animations/party_cylon_scanner_40_rainbow.mp4) |
| Random Sparkle | ![](SOFTWARE/animations/party_random_sparkle_15_rainbow.gif) [MP4](SOFTWARE/animations/party_random_sparkle_15_rainbow.mp4) | ![](SOFTWARE/animations/party_random_sparkle_16_rainbow.gif) [MP4](SOFTWARE/animations/party_random_sparkle_16_rainbow.mp4) | ![](SOFTWARE/animations/party_random_sparkle_4_rainbow.gif) [MP4](SOFTWARE/animations/party_random_sparkle_4_rainbow.mp4) | ![](SOFTWARE/animations/party_random_sparkle_24_rainbow.gif) [MP4](SOFTWARE/animations/party_random_sparkle_24_rainbow.mp4) | ![](SOFTWARE/animations/party_random_sparkle_32_rainbow.gif) [MP4](SOFTWARE/animations/party_random_sparkle_32_rainbow.mp4) | ![](SOFTWARE/animations/party_random_sparkle_40_rainbow.gif) [MP4](SOFTWARE/animations/party_random_sparkle_40_rainbow.mp4) |
| Beat Meter | ![](SOFTWARE/animations/party_beat_meter_15_rainbow.gif) [MP4](SOFTWARE/animations/party_beat_meter_15_rainbow.mp4) | ![](SOFTWARE/animations/party_beat_meter_16_rainbow.gif) [MP4](SOFTWARE/animations/party_beat_meter_16_rainbow.mp4) | ![](SOFTWARE/animations/party_beat_meter_4_rainbow.gif) [MP4](SOFTWARE/animations/party_beat_meter_4_rainbow.mp4) | ![](SOFTWARE/animations/party_beat_meter_24_rainbow.gif) [MP4](SOFTWARE/animations/party_beat_meter_24_rainbow.mp4) | ![](SOFTWARE/animations/party_beat_meter_32_rainbow.gif) [MP4](SOFTWARE/animations/party_beat_meter_32_rainbow.mp4) | ![](SOFTWARE/animations/party_beat_meter_40_rainbow.gif) [MP4](SOFTWARE/animations/party_beat_meter_40_rainbow.mp4) |

## ADJ1 Ring-Size Feedback

Turning ADJ1 while the pack is off confirms the selected cyclotron size on the ring (firmware v1.2.0 and later). Each setting has an unmistakable look, so the setting reads correctly even on rings with fewer physical LEDs than the selected count. Firmware prior to v1.2.0 showed the scrolling rainbow at every setting.

| 4 LEDs — solid red | 24 LEDs — solid green | 32 LEDs — solid blue | 40 LEDs — scrolling rainbow |
| --- | --- | --- | --- |
| ![](SOFTWARE/animations/ring_size_feedback_4_red.gif) [MP4](SOFTWARE/animations/ring_size_feedback_4_red.mp4) | ![](SOFTWARE/animations/ring_size_feedback_24_green.gif) [MP4](SOFTWARE/animations/ring_size_feedback_24_green.mp4) | ![](SOFTWARE/animations/ring_size_feedback_32_blue.gif) [MP4](SOFTWARE/animations/ring_size_feedback_32_blue.mp4) | ![](SOFTWARE/animations/ring_size_feedback_40_rainbow.gif) [MP4](SOFTWARE/animations/ring_size_feedback_40_rainbow.mp4) |

To regenerate the previews locally:

```
cmake -S SOFTWARE/sim -B SOFTWARE/sim/build
cmake --build SOFTWARE/sim/build --target sim_led
jq -c '.[]' SOFTWARE/sim/animation_configs.json | while read cfg; do
  name=$(echo "$cfg" | jq -r '.name')
  leds=$(echo "$cfg" | jq -r '.led')
  color=$(echo "$cfg" | jq -r '.color')
  layout=$(echo "$cfg" | jq -r '.layout')
  ANIMATION_NAME="$name" LED_COUNT="$leds" COLOR="$color" LAYOUT="$layout" SOFTWARE/sim/build/sim_led </dev/null
  ffmpeg -y -framerate 60 -i frames/frame_%05d.ppm \
    -c:v libx264 -pix_fmt yuv420p SOFTWARE/animations/${name}_${leds}_${color}.mp4
  ffmpeg -y -i SOFTWARE/animations/${name}_${leds}_${color}.mp4 \
    -vf "fps=15,scale=320:-1:flags=lanczos" \
    SOFTWARE/animations/${name}_${leds}_${color}.gif
  rm frames/frame_*.ppm
done
```
