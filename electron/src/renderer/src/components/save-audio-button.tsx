import { DownloadIcon } from 'lucide-react';
import { useState } from 'react';
import { useTranslation } from 'react-i18next';
import { toast } from 'sonner';
import { getBridge } from './bridge';
import { Button } from './ui/button';
import { saveExport } from '@/lib/export-history';

export function SaveAudioButton({
  url,
  suggestedName,
  disabled = false,
  iconOnly = false,
}: {
  url?: string;
  suggestedName: string;
  disabled?: boolean;
  /** A compact secondary action: the label becomes the tooltip. */
  iconOnly?: boolean;
}) {
  const { t } = useTranslation();
  const [saving, setSaving] = useState(false);
  const bridge = getBridge();

  const save = async () => {
    if (!url) return;
    setSaving(true);
    try {
      if (!bridge) {
        // Browser tab (web build, Colab tunnel): there is no native save
        // dialog to call, so hand the file to the browser's own downloader.
        // The URL is same-origin in a web deployment, so `download` keeps the
        // suggested name; a cross-origin one still downloads, just under the
        // server's filename.
        const link = document.createElement('a');
        link.href = url;
        link.download = suggestedName;
        link.rel = 'noopener';
        document.body.append(link);
        link.click();
        link.remove();
        return;
      }
      const saved = await saveExport(url, suggestedName);
      if (!saved) return;
      if (!saved.canceled && saved.path) {
        const path = saved.path;
        toast.success(t('clone.saved_to', { path }), {
          action: {
            label: t('clone.reveal'),
            onClick: () =>
              void bridge.files.revealPath(path).catch((error) => {
                const message = error instanceof Error ? error.message : String(error);
                toast.error(t('clone.download_failed', { message }));
              }),
          },
        });
      }
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      toast.error(t('clone.download_failed', { message }));
    } finally {
      setSaving(false);
    }
  };

  return (
    <Button
      variant="ghost"
      size={iconOnly ? 'icon-sm' : 'sm'}
      aria-label={iconOnly ? t('clone.download') : undefined}
      title={iconOnly ? t('clone.download') : undefined}
      onClick={() => void save()}
      disabled={disabled || saving || !url}
    >
      <DownloadIcon />
      {iconOnly ? null : t('clone.download')}
    </Button>
  );
}
