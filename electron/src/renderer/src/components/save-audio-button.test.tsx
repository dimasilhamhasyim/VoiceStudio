import { cleanup, fireEvent, render, screen } from '@testing-library/react';
import { afterEach, expect, it, vi } from 'vitest';

const mocks = vi.hoisted(() => ({
  saveExport: vi.fn(),
}));

vi.mock('react-i18next', () => ({
  useTranslation: () => ({ t: (key: string) => key }),
}));
vi.mock('sonner', () => ({ toast: { success: vi.fn(), error: vi.fn() } }));
vi.mock('@/lib/export-history', () => ({ saveExport: mocks.saveExport }));

import { SaveAudioButton } from './save-audio-button';

afterEach(() => {
  cleanup();
  vi.clearAllMocks();
  delete (window as { voicestudio?: unknown }).voicestudio;
});

it('downloads through the browser when no Electron bridge is present', () => {
  const click = vi.spyOn(HTMLAnchorElement.prototype, 'click').mockImplementation(() => {});
  render(<SaveAudioButton url="/audio/3fb5f264.wav" suggestedName="voicestudio-1.wav" />);

  const button = screen.getByRole('button', { name: 'clone.download' });
  expect(button).toBeEnabled();
  fireEvent.click(button);

  expect(click).toHaveBeenCalledTimes(1);
  const anchor = click.mock.instances[0] as unknown as HTMLAnchorElement;
  expect(anchor.getAttribute('href')).toBe('/audio/3fb5f264.wav');
  expect(anchor.getAttribute('download')).toBe('voicestudio-1.wav');
  expect(mocks.saveExport).not.toHaveBeenCalled();
  click.mockRestore();
});

it('uses the native save dialog when the Electron bridge is present', async () => {
  (window as { voicestudio?: unknown }).voicestudio = {
    files: { saveAudio: vi.fn(), saveData: vi.fn(), revealPath: vi.fn() },
  };
  mocks.saveExport.mockResolvedValue({ canceled: false, path: '/tmp/x.wav' });

  render(<SaveAudioButton url="/audio/3fb5f264.wav" suggestedName="voicestudio-1.wav" />);
  fireEvent.click(screen.getByRole('button', { name: 'clone.download' }));

  await vi.waitFor(() =>
    expect(mocks.saveExport).toHaveBeenCalledWith('/audio/3fb5f264.wav', 'voicestudio-1.wav'),
  );
});
