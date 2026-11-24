import { ReactNode, useEffect } from "react";

export default function Modal({ title, onClose, children }: { title: string; onClose: () => void; children: ReactNode }) {
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => e.key === "Escape" && onClose();
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [onClose]);
  return (
    <div className="fixed inset-0 z-20 flex items-center justify-center bg-black/60 px-4" onClick={onClose}>
      <div role="dialog" aria-modal="true" aria-label={title} className="card w-full max-w-lg space-y-4"
           onClick={(e) => e.stopPropagation()}>
        <h2 className="text-lg font-semibold text-slate-100">{title}</h2>
        {children}
      </div>
    </div>
  );
}
