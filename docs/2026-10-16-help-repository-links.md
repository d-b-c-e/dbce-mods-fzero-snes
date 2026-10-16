# Help links through existing Support

Source-only successor for owner179543, based on frozen5f0202e. The actual
F-Zero Mods provider now supplies two named repository links on its existing
Support/Diagnostics feature. The existing paired recomp-ui Mods detail renderer
places them under a muted Help label as ordinary text hyperlinks:

- Mod on GitHub: https://github.com/d-b-c-e/dbce-mods-fzero-snes
- Wheelkit on GitHub: https://github.com/d-b-c-e/dbce-wheelkit

Help remains visible when Diagnostics is off. Diagnostics itself, feature
identities/counts, options, saved settings, provider mutations, input ownership
and force behavior are unchanged. There is no new feature checkbox or extra
panel. The renderer reuses the existing ImGui::TextLinkOpenURL widget/opener;
opening a page still requires an explicit user click. No URL was opened.

The paired UI revision is53e9350b81aca02fc9b4c2fbb3d795ea621bea6f at
`E:/Source/games/.worktrees/dine-fzero-help-links-ui-20261016`.
Its optional fixed-buffer Help metadata extends the feature source descriptor.
Build host and UI together from that gitlink; do not combine an old host image
with the new UI or relabel the retained D37 build. Existing zero-initialized
features omit the Help subsection, and the renderer bounds the count and skips
missing labels/URLs. This follows the reviewed small-link presentation contract
of toolkit e8fca59 while using F-Zero's own existing renderer/provider, not the
native dbce_panel that F-Zero does not consume.

## Existing issue-report flow and remaining gap

The qualified bundled helper sourceb887 has only Controls/Screens/Settings tabs
in `src/Wheelkit.StandaloneSetup.Gui/SetupWindow.xaml`. Its Program parses the
strict existing setup/wait contract; no report command or deeplink is exposed.
F-Zero's Mods provider also has no existing report callback/transport.

The full Wheelkit App already owns the reviewed flow:
`src/Wheelkit.App/ViewModels/GameViewModel.Reporting.cs:7-12` opens
ProblemReportWindow with the selected mod repository, name, version and log.
`App.xaml.cs --support-file` produces a support ZIP; it is not that selected-game
issue-review/submission route. Neither is a current direct callable Help action
from this launcher/helper. No report generator, uploader, browser issue draft,
new CLI/deeplink or second report window was recreated. Direct access to that
existing selected-game flow remains a separate integration gap.

## Qualification boundary

Source diff review and whitespace checks only. No compiler, fixture, helper or
game launch, native device, URL opener, installer, owner file or public push was
used. The next scheduled affected batch should build host plus this paired UI
and run the existing provider/CAPI/model gates. Verify rendered Help links and
unchanged Diagnostics in the later useful combined helper/Play session rather
than creating a separate unchanged-baseline game run.

Frozen5f/D37/UI217/helperb887 and the FZERO-DEFAULT-INSTALL-02 packet retain their
original bytes and qualification scope. This source successor is not installed
and claims no rendered or binary qualification.
