# 5cript-nui-components

Reusable components that I use with NuiCpp.
These have been created out of frustration with horribly performing component libs.
Not part of NuiCpp because I am not upholding the quality bar for nui here, nor do I strive for completeness, just what I need for my projects.
Also I am not a great ui designer.

## Use in project

CMake:
```cmake
include(FetchContent)

FetchContent(
	script-nui-components
	GIT_REPOSITORY https://github.com/NuiCpp/Nui.git
	GIT_TAG        v2.0.0
)
FetchContent_MakeAvailable(script-nui-components)

target_link_libraries(your_target PRIVATE script-nui-components)
```

## Styling

The components use CSS variables for styling, which can be set on the component itself or globally.

## Components

### Switch

#### C++

```cpp
#include <script-nui-components/switch.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    // isChecked_ is Nui::Observed<bool>, std::shared_ptr<Nui::Observed<bool>>, or bool.

    return body{}(
        switch_(SwitchOptions<decltype(isChecked_)>{
            .isChecked = isChecked_,
            .sizeFactor = 1,
            .colorActive = "#008000",
            .colorInactive = std::nullopt,
            .thumbColor = std::nullopt,
            .onChange = [this](bool isChecked, Nui::WebApi::MouseEvent const&)
            {
                Nui::WebApi::Console::log(fmt::format("Switch is now {}", isChecked));
            },
            .attributes = {
                // and others...
                .disabled = false
            }
        })
    );
}
```

#### Css Variables

```css
:root {
    /* Size factor for the switch, where 1 is the default size. This scales the entire switch, including the thumb. */
    --script-nui-components-switch-size-factor: 1;

    /* Colors for the switch. The active color is used when the switch is on, and the inactive color is used when the switch is off. The thumb color is used for the circular thumb of the switch. */
    --script-nui-components-switch-color-active: #008000;
    --script-nui-components-switch-color-inactive: #1f2228;
    --script-nui-components-switch-thumb-color: #dddddd;
}
```

### Button

#### C++

```cpp
#include <script-nui-components/button.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    return body{}(
        return button(ButtonOptions{
            .text = "Icon Button",
            .icon =
                Nui::Elements::Svg::svg{
                    Nui::Attributes::Svg::viewBox = "0 0 24 24"s,
                }(Nui::Elements::Svg::path{
                    Nui::Attributes::Svg::d = "m6 9 6 6 6-6"s,
                }()),
            .attributes = {
                Nui::Attributes::onClick = [](auto const&)
                {
                    Nui::WebApi::Console::log("Icon button clicked!");
                }
            }
        });
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-button-color-primary: #5fbcff;
    --script-nui-components-button-color-warning: #fbbf24;
    --script-nui-components-button-color-danger: #dc2626;
    --script-nui-components-button-color-regular: #202020;
    --script-nui-components-button-color-disabled: #505050;
    --script-nui-components-button-border-color: #505050;
    --script-nui-components-button-color: #dddddd;
    --script-nui-components-button-font: inherit;
    --script-nui-components-button-font-size: 0.8rem;
    --script-nui-components-button-border-radius: 0px;
}
```

### Text Input

#### C++

```cpp
#include <script-nui-components/text_input.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    // value is Nui::Observed<std::string>, std::shared_ptr<Nui::Observed<std::string>>, or std::string.

    return body{}(
        textInput(TextInputOptions<decltype(value_)>{
            .value = value_,
            .onChange = [this](std::string const& newValue, Nui::WebApi::InputEvent const&)
            {
                Nui::WebApi::Console::log(fmt::format("Text input is now {}", newValue));
            },
            .attributes = {
                // and others...
                .disabled = false
            }
        })
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-field-border-radius: 0px;
    --script-nui-components-field-color: #dddddd;
    --script-nui-components-field-background-color: #202020;
    --script-nui-components-field-font: inherit;
    --script-nui-components-field-font-size: 0.8rem;
    --script-nui-components-field-theme-color: #5fbcff;
    --script-nui-components-field-hover-shadow: 0 0 0 0.1rem var(--script-nui-components-field-theme-color);
    --script-nui-components-field-border-color: #505050;
    --script-nui-components-field-focus-shadow: var(--script-nui-components-field-hover-shadow);
    /* Used for invalid state of the input, e.g. when pattern attribute is not matched. */
    --script-nui-components-field-color-invalid: #dc2626;
}
```

### Select

#### C++

```cpp
#include <script-nui-components/select.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    // MainPage members:
    //
    // Nui::Observed<std::string> selectValue_{};
    // Nui::Observed<std::vector<std::string>> selectOptions_{
    //     std::vector<std::string>{"Option 1", "Option 2", "Option 3"},
    // };

    return body{}(
        select(SelectOptions<decltype(selectValue_), decltype(selectOptions_)>{
            .activeOption = selectValue_,
            .options = selectOptions_,
            .attributes = {
                // and others...
                .disabled = false
            },
            // Optional muted text inside the box in front of the active option. Also takes an Observed string.
            .label = "Fruit",
            // Optional icon in front of the label or the active option. Either one can be used alone.
            .icon = Ui5Icons::nutrition_activity(),
            .onChange = [this](std::string const& newValue, auto const&)
            {
                Nui::WebApi::Console::log("Selected option: "s + newValue);
            },
            // Creates ui elements active element, can be used to show icons or images in addition to text.
            // Default is a span.
            .activeRenderer = [](Nui::Observed<std::string> const& option) -> Nui::ElementRenderer
            {
                return span{}(option);
            },
            // Creates ui elements from vector elements, can be used to show icons or images in addition to text.
            // Default is a span.
            .elementRenderer = [](std::string const& option) -> Nui::ElementRenderer
            {
                return span{}(option);
            }
        })
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-field-border-radius: 0px;
    --script-nui-components-field-color: #dddddd;
    --script-nui-components-field-background-color: #202020;
    --script-nui-components-field-font: inherit;
    --script-nui-components-field-font-size: 0.8rem;
    --script-nui-components-field-theme-color: #5fbcff;
    --script-nui-components-field-hover-shadow: 0 0 0 0.1rem var(--script-nui-components-field-theme-color);
    --script-nui-components-field-border-color: #505050;
    --script-nui-components-field-focus-shadow: var(--script-nui-components-field-hover-shadow);
}
```

### Color Picker

#### C++

```cpp
#include <script-nui-components/color_picker.hpp>

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    // MainPage members:
    //
    // Nui::Observed<std::string> colorValue_{};

    return body{}(
        return colorPicker(ColorPickerOptions<decltype(colorValue_)>{
            .value = colorValue_,
            .pickButtonText = "",
            .pickButtonIcon = Nui::Elements::Svg::svg{
                Nui::Attributes::Svg::viewBox = "0 0 24 24",
            }(Nui::Elements::Svg::path{
                Nui::Attributes::Svg::d = "" /* something meaningful */,
            }()),
            .buttonStyleVariant = StyleVariant::Transparent
        });
    );
}
```

#### Css Variables

Styling is determined by button and text field styles.

### Resizeable Table

This component does not self holds its state and needs to be instantiated outside the render flow.
It does not hold the table model, but relies on the user to add / remove rows.

#### C++

```cpp
#include <script-nui-components/resizeable_table.hpp>

MainPage::MainPage()
    : table_{
          ResizableTable::HeaderRow{
              // Resizeable with dynamic width:
              ResizableTable::HeaderTableCell{"Header 1"s},
              ResizableTable::HeaderTableCell{"Header 2"s},
              ResizableTable::HeaderTableCell{
                  .content = "",
                  .initialWidth = 40,
                  .resizeable = false,
              }
          },
          // or nullopt for no footer
          ResizableTable::FooterFeature{{"Footer 1"s}, {"Footer 2"s}, {""s}},
          // or nullopt for no add row:
          ResizableTable::AddFeature{
              .onAdd =
                  [](auto& table)
              {
                  const auto rowCount = table.rowCount();
                  table.addRow({
                      fmt::format("Row {}, Col 1", rowCount),
                      fmt::format("Row {}, Col 2", rowCount),
                      [](std::unique_ptr<ResizableTable::ISelfController> controller) -> Nui::ElementRenderer
                      {
                          // std::function must be copiable
                          std::shared_ptr<ResizableTable::ISelfController> sharedController = std::move(controller);

                          return button(ButtonOptions{
                              .icon =
                                  Nui::Elements::Svg::svg{
                                      Nui::Attributes::Svg::viewBox = "0 0 512 512",
                                  }(Nui::Elements::Svg::path{
                                      Nui::Attributes::Svg::d =
                                          "M454 109q11 0 18.5 7t7.5 18-7.5 18.5T454 160h-19v294q0 24-17 41t-41 "
                                          "17H135q-24 0-41-17t-17-41V160H58q-11 0-18.5-7.5T32 134t7.5-18 "
                                          "18.5-7h70V58q0-24 17-41t41-17h140q24 0 41 17t17 41v51h70zm-275 "
                                          "0h154V58q0-7-7-7H186q-7 0-7 7v51zm205 51H128v294q0 7 7 7h242q7 0 "
                                          "7-7V160zm-186 64q11 0 18.5 7.5T224 250v140q0 11-7.5 18.5T198 "
                                          "416t-18-7.5-7-18.5V250q0-11 7-18.5t18-7.5zm116 0q11 0 18 7.5t7 18.5v140q0 "
                                          "11-7 18.5t-18 7.5-18.5-7.5T288 390V250q0-11 7.5-18.5T314 224z",
                                  }()),
                              .attributes = {
                                  Nui::Attributes::onClick = [controller = std::move(sharedController)](auto const&)
                                  {
                                      Nui::WebApi::Console::log(
                                          "Button in row "s + std::to_string(controller->row()) + " clicked!"
                                      );
                                      controller->remove();
                                      // myState.erase(controller->row());
                                  }
                              }
                          });
                      },
                  });
              },
              .addNewEntryText = "Add new row",
          }
      }
{}

Nui::ElementRenderer MainPage::render()
{
    using namespace ScriptNuiComponents;

    return body{}(
        return table_({
            .disabled = false
        })
    );
}
```

#### Css Variables

```c++
:root {
    --script-nui-components-table-border-radius: 10px;
    --script-nui-components-table-font-size: 0.85rem;
    --script-nui-components-table-font: system-ui, -apple-system, sans-serif;

    --script-nui-components-table-color: #c9d1d9;
    --script-nui-components-table-header-color: #e6edf3;
    --script-nui-components-table-footer-color: #8b949e;

    --script-nui-components-table-cell-background: rgb(30, 30, 30);
    --script-nui-components-table-header-background: linear-gradient(180deg, var(--script-nui-components-table-cell-background), color-mix(in srgb, var(--script-nui-components-table-cell-background), black 40%));
    --script-nui-components-table-footer-background: color-mix(in srgb, var(--script-nui-components-table-cell-background), white 10%);

    --script-nui-components-table-border-color: #444444;
    --script-nui-components-table-row-hover: color-mix(in srgb, var(--script-nui-components-table-cell-background), white 10%);

    --script-nui-components-table-handle-color: rgb(90, 90, 90);
    --script-nui-components-table-handle-active: #0078d4;
}
```

### Progress Color

Header only. The hue curve the progress bar and the progress ring use in `ProgressColorMode::Hue`.

#### C++

```cpp
#include <script-nui-components/progress_color.hpp>

using namespace ScriptNuiComponents;

// 120 * fraction^1.8, at least 5, and 120 once the fraction reaches 1.
const double hue = progressHue(0.5);
// "hsl(34, 40%, 45%)", the hue truncated to whole degrees.
const std::string color = progressHueColor(0.5);
// "hue" or "theme", the value of data-color-mode.
const std::string mode = toString(ProgressColorMode::Theme);
```

#### Css Variables

None.

### Progress Bar

A horizontal bar with a fill, a text and a shine. `progressBar` binds to shared observed values; the `ProgressBar`
class is driven by setters and renders the same markup. Setters never sync: call
`Nui::globalEventContext.executeActiveEventsImmediately()` once after changing them.

Markup: `div.script-nui-progress-bar[role=progressbar][data-color-mode][data-complete]` holding
`div.script-nui-progress-bar-fill`, `div.script-nui-progress-bar-text` and `div.script-nui-progress-bar-shine`, in this
order. `data-color-mode` is `hue` or `theme`, `data-complete` is present once current reached maximum.
`aria-valuenow` carries the whole percentage and `aria-valuetext` the text.

#### C++

```cpp
#include <script-nui-components/progress_bar.hpp>

using namespace ScriptNuiComponents;

// MainPage members:
//
// std::shared_ptr<Nui::Observed<std::int64_t>> done_{std::make_shared<Nui::Observed<std::int64_t>>(0)};
// std::shared_ptr<Nui::Observed<std::int64_t>> total_{std::make_shared<Nui::Observed<std::int64_t>>(4096)};
// ProgressBar bar_{{
//     .height = "11px",
//     .minimum = 0,
//     .maximum = 0,
//     .showMinimumAndMaximum = true,
//     .byteMode = true,
// }};

Nui::ElementRenderer MainPage::render()
{
    return body{}(
        progressBar({
            .current = done_,
            .maximum = total_,
            .minimum = 0,
            // None, Percent ("{:.1f}%"), Counts ("{} / {} ({:.1f}%)") or Bytes ("{} - {} ({:.1f}%)").
            .textMode = ProgressBarTextMode::Bytes,
            // Hue colors the fill from red to green, Theme uses the theme variable.
            .colorMode = ProgressColorMode::Hue,
            // A bar from 0 to 0 fills completely instead of staying empty.
            .zeroAsComplete = false,
            // Runs the shine animation while the bar is not complete.
            .shine = true,
            // Overrides --script-nui-components-progress-bar-height.
            .height = "11px",
            // Replaces the text of Percent, Counts and Bytes.
            .formatText = [](ProgressBarValues const& values) {
                return fmt::format("{:.0f} %", values.fraction * 100.0);
            },
            // The format strings of the built-in texts, English defaults.
            .texts = {},
            .attributes = {},
        }),
        bar_()
    );
}

void MainPage::onProgress(std::int64_t done, std::int64_t total)
{
    bar_.setMaximum(total);
    bar_.setProgress(done);
    // Also: setZeroAsComplete(), setTextMode(ProgressBarTextMode::Counts), progress(), maximum(), textMode().
    Nui::globalEventContext.executeActiveEventsImmediately();
}
```

#### Css Variables

```css
:root {
    --script-nui-components-progress-bar-height: 30px;
    --script-nui-components-progress-bar-track: #606060;
    --script-nui-components-progress-bar-radius: 15px;
    --script-nui-components-progress-bar-text-color: #303030;
    --script-nui-components-progress-bar-text-shadow: 0 0 2px rgba(0, 0, 0, 0.5);
    --script-nui-components-progress-bar-font: inherit;
    --script-nui-components-progress-bar-font-size: inherit;
    --script-nui-components-progress-bar-font-weight: bold;
    /* Fill color in ProgressColorMode::Theme. */
    --script-nui-components-progress-bar-theme: #4caf50;
    --script-nui-components-progress-bar-fill-transition: none;
    /* Highlight of the shine gradient; prefers-reduced-motion stops the shine. */
    --script-nui-components-progress-bar-shine: rgba(255, 255, 255, 0.5);
    --script-nui-components-progress-bar-shine-duration: 2s;
}
```

### Progress Ring

A small circular indicator for rows and headers: an svg with a track and a fill circle (`pathLength` 100) whose
`stroke-dashoffset` and, in Hue mode, `stroke` follow the fraction.

Markup: `svg.script-nui-progress-ring[role=progressbar][data-color-mode][data-complete][data-indeterminate]` holding
`circle.script-nui-progress-ring-track` and `circle.script-nui-progress-ring-fill`.

#### C++

```cpp
#include <script-nui-components/progress_ring.hpp>

using namespace ScriptNuiComponents;

// MainPage members:
//
// std::shared_ptr<Nui::Observed<double>> fraction_{std::make_shared<Nui::Observed<double>>(0.25)};
// Nui::Observed<bool> waiting_{false};

Nui::ElementRenderer MainPage::render()
{
    return body{}(
        progressRing({
            // From 0 to 1.
            .fraction = fraction_,
            .colorMode = ProgressColorMode::Hue,
            // Spins a fixed arc in the theme color instead; a bool, an Observed<bool> or a combinator.
            .indeterminate = waiting_,
            // aria-label.
            .label = "Uploading",
            .attributes = {},
        })
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-progress-ring-size: 16px;
    /* In units of the 24 unit viewBox; the circles have a radius of 9. */
    --script-nui-components-progress-ring-stroke-width: 3;
    --script-nui-components-progress-ring-track: #505050;
    /* Fill color in ProgressColorMode::Theme. */
    --script-nui-components-progress-ring-theme: #4caf50;
    --script-nui-components-progress-ring-transition: stroke-dashoffset 200ms linear, stroke 200ms linear;
    /* One turn while indeterminate; prefers-reduced-motion stops it. */
    --script-nui-components-progress-ring-spin-duration: 1s;
}
```

### Badge

A small rounded label for counts and states. `countBadge` shows a count and hides itself at 0.

The markup is a contract, so code outside Nui (for example a TypeScript tab renderer) can produce the same badge with
nothing but the stylesheet:

```html
<span class="script-nui-badge" data-variant="warning">3</span>
<span class="script-nui-badge" data-variant="danger" data-hidden>0</span>
<span class="script-nui-badge" data-variant="primary" data-dot></span>
```

- `data-variant`: `regular`, `primary`, `warning`, `danger`, `success` or `transparent`; missing looks like `regular`.
- `data-hidden`, `data-dot`: presence only, the value is ignored.
- `data-pop`: optional, `a` or `b`; switching between them replays the pop animation.
- The text is the only child; a dot badge has none.

#### C++

```cpp
#include <script-nui-components/badge.hpp>

using namespace ScriptNuiComponents;

// MainPage members:
//
// Nui::Observed<std::size_t> conflicts_{0u};
// Nui::Observed<int> pop_{0};

Nui::ElementRenderer MainPage::render()
{
    return body{}(
        badge({
            // A string, an Observed<std::string> or a combinator.
            .text = "Syncing",
            // A StyleVariant, an Observed<StyleVariant> or a combinator.
            .styleVariant = StyleVariant::Primary,
            // A bool, an Observed<bool> or a combinator.
            .hidden = false,
            .dot = false,
            // Increment pop_ to replay the pop animation; 0 never pops.
            .pop = &pop_,
            .attributes = {},
        }),
        // An integer, an Observed of one (reference or shared_ptr) or a combinator generating one.
        countBadge(conflicts_, StyleVariant::Warning)
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-badge-font: inherit;
    --script-nui-components-badge-font-size: 0.7rem;
    --script-nui-components-badge-font-weight: 600;
    /* Also the minimum width, so short counts are round. */
    --script-nui-components-badge-height: 1.5em;
    --script-nui-components-badge-radius: 999px;
    --script-nui-components-badge-dot-size: 0.5rem;
    /* The pop grows the badge in from nothing; prefers-reduced-motion stops it. */
    --script-nui-components-badge-pop-duration: 450ms;

    --script-nui-components-badge-regular-background: #3b3b3b;
    --script-nui-components-badge-regular-color: #d6d6d6;
    --script-nui-components-badge-primary-background: #185fa5;
    --script-nui-components-badge-primary-color: #ffffff;
    --script-nui-components-badge-warning-background: #e0a02e;
    --script-nui-components-badge-warning-color: #1a1a1a;
    --script-nui-components-badge-danger-background: #d9534f;
    --script-nui-components-badge-danger-color: #ffffff;
    --script-nui-components-badge-success-background: #7cb342;
    --script-nui-components-badge-success-color: #1a1a1a;
    /* The transparent variant has no background. */
    --script-nui-components-badge-transparent-color: currentColor;
}
```

### Tab Strip

A row of fixed tabs with optional count badges. Unlike `Tabs` it does not drag, close or own its tabs; the caller
owns the selection.

Markup: `div.script-nui-tab-strip[role=tablist]` holding one
`button.script-nui-tab-strip-tab[role=tab][data-tab-key][aria-selected]` per tab, `data-selected` on the selected
one, each with `span.script-nui-tab-strip-title` and the count badge. Left / Right (wrapping), Home and End move the
selection and the focus between tabs. Only the selected tab (or the first one while none is) is in the tab order.

#### C++

```cpp
#include <script-nui-components/tab_strip.hpp>

using namespace ScriptNuiComponents;

// MainPage members:
//
// std::shared_ptr<Nui::Observed<std::string>> detailTab_{std::make_shared<Nui::Observed<std::string>>("conflicts")};
// Nui::Observed<std::size_t> conflicts_{0u};

Nui::ElementRenderer MainPage::render()
{
    return body{}(
        tabStrip({
            .tabs = {
                {
                    .key = "conflicts",
                    // A string, an Observed<std::string> or a combinator.
                    .title = "Conflicts",
                    // Hidden while the count is 0.
                    .badge = TabStripBadge{.count = conflicts_, .styleVariant = StyleVariant::Warning},
                    .attributes = {},
                },
                {.key = "options", .title = "Options"},
            },
            // The strip assigns the key of the activated tab, then calls onSelect.
            .selected = detailTab_,
            .onSelect = [](std::string const& key) {
                Nui::WebApi::Console::log("Selected ", key);
            },
            // aria-label of the tab list.
            .label = "Details",
            .attributes = {},
        })
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-tab-strip-color: #a0a0a0;
    --script-nui-components-tab-strip-color-selected: #e6e6e6;
    /* Underline of the selected tab and the focus ring. */
    --script-nui-components-tab-strip-accent-color: #90d26d;
    --script-nui-components-tab-strip-background: transparent;
    --script-nui-components-tab-strip-hover-background: rgba(255, 255, 255, 0.06);
    --script-nui-components-tab-strip-border-color: #3a3a3a;
    --script-nui-components-tab-strip-font: inherit;
    --script-nui-components-tab-strip-font-size: 0.8rem;
    --script-nui-components-tab-strip-gap: 0.25rem;
}
```

### List Row

A selectable row for lists and outliners, meant to sit in an element with `role=listbox`.

Markup: `div.script-nui-list-row[role=option][tabindex=0][aria-selected]`, `data-selected` while selected, holding
`span.script-nui-list-row-leading` (only with `leading`), `span.script-nui-list-row-label`,
`span.script-nui-list-row-side` and `span.script-nui-list-row-trailing` (only with `trailing`). Clicks and keys inside
the trailing part never activate the row.

#### C++

```cpp
#include <script-nui-components/list_row.hpp>

using namespace ScriptNuiComponents;

// MainPage members:
//
// Nui::Observed<int> selected_{0};

Nui::ElementRenderer MainPage::render()
{
    return body{}(
        div{"role"_attr = "listbox"}(
            listRow({
                .leading = Ui5Icons::document(),
                // Strings, Observed<std::string> or combinators.
                .label = "notes.txt",
                .sideText = "3 changes",
                // Clicks in here do not reach the row.
                .trailing = button({.icon = Ui5Icons::delete_(), .styleVariant = StyleVariant::Transparent}),
                // A bool, an Observed<bool> or a combinator.
                .selected = Nui::observe(selected_).generate([this]() { return selected_.value() == 0; }),
                // Click, Enter or Space.
                .onActivate = [this]() { selected_ = 0; },
                .attributes = {},
            })
        )
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-list-row-color: #dddddd;
    --script-nui-components-list-row-side-color: #9a9a9a;
    --script-nui-components-list-row-hover-background: rgba(255, 255, 255, 0.05);
    --script-nui-components-list-row-selected-background: rgba(74, 158, 218, 0.22);
    --script-nui-components-list-row-selected-color: #ffffff;
    --script-nui-components-list-row-focus-color: #4a9eda;
    /* Minimum height. */
    --script-nui-components-list-row-height: 1.9rem;
    --script-nui-components-list-row-padding: 0 0.5rem;
    --script-nui-components-list-row-gap: 0.5rem;
    --script-nui-components-list-row-radius: 4px;
    --script-nui-components-list-row-font-size: 0.8rem;
}
```

### Collapsible Section

A titled section that folds (native `details` / `summary`). `headerEnd` puts content at the end of the header, in
`span.script-nui-collapsible-header-end`; clicks in it never fold the section.

#### C++

```cpp
#include <script-nui-components/collapsible_section.hpp>

using namespace ScriptNuiComponents;

// MainPage members:
//
// Nui::Observed<std::size_t> stopped_{2u};

Nui::ElementRenderer MainPage::render()
{
    return body{}(
        collapsibleSection(
            {
                // A string, an Observed<std::string> or a combinator.
                .title = "Stopped",
                // Small text badge after the title.
                .badge = std::nullopt,
                // Clicks in here are default-prevented, so they never fold the section.
                .headerEnd = countBadge(stopped_, StyleVariant::Regular),
                .initiallyExpanded = true,
                .onToggle = [](bool expanded) {},
                .attributes = {},
            },
            div{}("Content")
        )
    );
}
```

#### Css Variables

```css
:root {
    --script-nui-components-collapsible-color: #dddddd;
    --script-nui-components-collapsible-muted-color: color-mix(in srgb, var(--color) 55%, transparent);
    --script-nui-components-collapsible-badge-color: #c8c8c8;
    --script-nui-components-collapsible-badge-background: #2f2f2f;
    --script-nui-components-collapsible-hover-background: color-mix(in srgb, var(--color) 8%, transparent);
    --script-nui-components-collapsible-font-size: 0.85rem;
}
```

### Message Strip

A highlighted message with an icon. `actions` puts controls after the text, in `span.script-nui-message-strip-actions`.

#### C++

```cpp
#include <script-nui-components/message_strip.hpp>

using namespace ScriptNuiComponents;

Nui::ElementRenderer MainPage::render()
{
    return body{}(
        messageStrip({
            // A string, an Observed<std::string> or a combinator.
            .text = "Stopped: another watch covers this folder.",
            .styleVariant = StyleVariant::Warning,
            // Defaults to an icon matching the variant.
            .icon = Ui5Icons::warning(),
            .actions = button({.text = "Show it", .styleVariant = StyleVariant::Transparent}),
            .attributes = {},
        })
    );
}
```

#### Css Variables

```css
:root {
    /* Per variant: regular, primary, warning, danger and success. */
    --script-nui-message-strip-warning-bg: #412402;
    --script-nui-message-strip-warning-border-color: #854F0B;
    --script-nui-message-strip-warning-accent-color: #EF9F27;
    --script-nui-message-strip-warning-text-color: #FAC775;
    --script-nui-message-strip-warning-icon-color: #EF9F27;

    --script-nui-message-strip-regular-bg: #2C2C2A;
    --script-nui-message-strip-regular-border-color: #5F5E5A;
    --script-nui-message-strip-regular-accent-color: #B4B2A9;
    --script-nui-message-strip-regular-text-color: #D3D1C7;
    --script-nui-message-strip-regular-icon-color: #B4B2A9;

    --script-nui-message-strip-primary-bg: #042C53;
    --script-nui-message-strip-primary-border-color: #185FA5;
    --script-nui-message-strip-primary-accent-color: #85B7EB;
    --script-nui-message-strip-primary-text-color: #B5D4F4;
    --script-nui-message-strip-primary-icon-color: #85B7EB;

    --script-nui-message-strip-danger-bg: #501313;
    --script-nui-message-strip-danger-border-color: #A32D2D;
    --script-nui-message-strip-danger-accent-color: #F09595;
    --script-nui-message-strip-danger-text-color: #F7C1C1;
    --script-nui-message-strip-danger-icon-color: #F09595;

    --script-nui-message-strip-success-bg: #173404;
    --script-nui-message-strip-success-border-color: #3B6D11;
    --script-nui-message-strip-success-accent-color: #97C459;
    --script-nui-message-strip-success-text-color: #C0DD97;
    --script-nui-message-strip-success-icon-color: #97C459;

    /* Without a variant match (transparent): */
    --script-nui-message-strip-bg: transparent;
    --script-nui-message-strip-border-color: currentColor;
    --script-nui-message-strip-accent-color: currentColor;
    --script-nui-message-strip-text-color: inherit;
    --script-nui-message-strip-icon-color: currentColor;
}
```
