export default function StatusDot({ online }: { online: boolean }) {
  return (
    <span className="inline-flex items-center gap-1.5 text-xs font-semibold">
      <span className={`h-2 w-2 rounded-full ${online ? "bg-emerald-400 shadow-[0_0_8px] shadow-emerald-400/60" : "bg-slate-600"}`} />
      <span className={online ? "text-emerald-300" : "text-slate-500"}>{online ? "online" : "offline"}</span>
    </span>
  );
}
