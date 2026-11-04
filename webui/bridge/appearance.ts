export type Theme = 'system' | 'light' | 'dark';
export type SupportedLanguage = 'en' | 'id' | 'zh' | 'ru' | 'pt' | 'es';

export interface Appearance {
  theme: Theme;
  reduceMotion: boolean;
  language?: SupportedLanguage;
}

export const APPEARANCE_KEY = 'coreflow.appearance.v1';
export const LANGUAGE_KEY = 'coreflow.language.v1';

export const LANGUAGES: { code: SupportedLanguage; label: string; flag: string }[] = [
  { code: 'en', label: 'English', flag: '🇺🇸' },
  { code: 'id', label: 'Indonesia', flag: '🇮🇩' },
  { code: 'zh', label: '简体中文', flag: '🇨🇳' },
  { code: 'ru', label: 'Русский', flag: '🇷🇺' },
  { code: 'pt', label: 'Português', flag: '🇧🇷' },
  { code: 'es', label: 'Español', flag: '🇪🇸' },
];

export const TRANSLATIONS: Record<SupportedLanguage, Record<string, string>> = {
  en: {
    appTitle: 'CFAE System Framework',
    appSubtitle: 'v3.0.R · Autonomous Mutation Safety & Equilibrium',
    stateBalanced: 'Balanced',
    stateSustained: 'Sustained Load',
    statePressure: 'Pressure Guard',
    stateThermal: 'Thermal Guard',
    daemonRunning: 'Engine Active',
    daemonStandby: 'Engine Standby',
    startDaemon: 'Start Framework',
    stopDaemon: 'Stop Framework',
    restoreBaseline: 'Restore OEM Baseline',
    studioModalBtn: 'Blueprint Studio',
    thermalMatrixTitle: 'Thermal & PSI Matrix',
    autonomousTuningTitle: 'Autonomous Tuning & Bias',
    longevityTitle: 'Hardware Longevity',
    developerConsoleTitle: 'Developer Console',
    liveStreamTitle: 'Live Kernel Reaction Stream',
    batteryStepCharging: 'Battery Step-Charging',
    storagePacing: 'UFS Burst Pacing',
    adaptiveBias: 'Adaptive Workload Bias',
    balancedProfile: 'Balanced (Responsive)',
    efficiencyProfile: 'Efficiency (Battery Saver)',
    gamingProfile: 'Gaming (Low Jitter)',
    multitaskingProfile: 'Multitasking (Headroom)',
    verifiedRecovery: 'Verified Recovery',
    activeMutations: 'Active Mutations',
    journalSafe: 'Safety Journal',
    refresh: 'Refresh',
    saving: 'Saving...',
    saveArmed: 'Armed live in kernel',
    language: 'Language',
    theme: 'Theme',
    system: 'System',
    light: 'Light',
    dark: 'Dark',
    reduceMotion: 'Reduce Motion',
    oemBaseline: 'OEM baseline',
    engineActive: 'Engine Active',
    enginePaused: 'Engine Paused',
    quickSettings: 'Quick Settings',
    safeRecoveryTitle: 'Baseline Guardian',
  },
  id: {
    appTitle: 'CFAE System Framework',
    appSubtitle: 'v3.0.R · Keamanan Mutasi Otonom & Ekuilibrium',
    stateBalanced: 'Seimbang',
    stateSustained: 'Beban Berkelanjutan',
    statePressure: 'Pelindung Tekanan',
    stateThermal: 'Pelindung Suhu',
    daemonRunning: 'Mesin Aktif',
    daemonStandby: 'Mesin Standby',
    startDaemon: 'Jalankan Framework',
    stopDaemon: 'Hentikan Framework',
    restoreBaseline: 'Pulihkan Baseline OEM',
    studioModalBtn: 'Studio Blueprint',
    thermalMatrixTitle: 'Matriks Suhu & PSI',
    autonomousTuningTitle: 'Penyetelan Otonom & Bias',
    longevityTitle: 'Masa Pakai Perangkat (Longevity)',
    developerConsoleTitle: 'Konsol Pengembang',
    liveStreamTitle: 'Siaran Reaksi Kernel Langsung',
    batteryStepCharging: 'Pengisian Baterai Bertahap',
    storagePacing: 'Pembatasan Lonjakan UFS',
    adaptiveBias: 'Bias Beban Adaptif',
    balancedProfile: 'Seimbang (Responsif)',
    efficiencyProfile: 'Efisiensi (Hemat Daya)',
    gamingProfile: 'Gaming (Jitter Rendah)',
    multitaskingProfile: 'Multitasking (Kapasitas Luas)',
    verifiedRecovery: 'Pemulihan Terverifikasi',
    activeMutations: 'Mutasi Aktif',
    journalSafe: 'Jurnal Keamanan',
    refresh: 'Segarkan',
    saving: 'Menyimpan...',
    saveArmed: 'Aktif langsung di kernel',
    language: 'Bahasa',
    theme: 'Tema',
    system: 'Sistem',
    light: 'Terang',
    dark: 'Gelap',
    reduceMotion: 'Kurangi Animasi',
    oemBaseline: 'Baseline OEM',
    engineActive: 'Mesin Aktif',
    enginePaused: 'Mesin Dijeda',
    quickSettings: 'Pengaturan Cepat',
    safeRecoveryTitle: 'Pelindung Baseline',
  },
  zh: {
    appTitle: 'CFAE System Framework',
    appSubtitle: 'v3.0.R · 自主变异安全与平衡架构',
    stateBalanced: '均衡模式',
    stateSustained: '持续负载',
    statePressure: '压力防护',
    stateThermal: '温控保护',
    daemonRunning: '引擎运行中',
    daemonStandby: '引擎待机',
    startDaemon: '启动框架',
    stopDaemon: '停止框架',
    restoreBaseline: '恢复出厂基线',
    studioModalBtn: '架构工作室',
    thermalMatrixTitle: '温度与 PSI 矩阵',
    autonomousTuningTitle: '自主调优与偏好',
    longevityTitle: '硬件寿命保护',
    developerConsoleTitle: '开发者控制台',
    liveStreamTitle: '内核实时反应流',
    batteryStepCharging: '阶梯式电池充电',
    storagePacing: 'UFS 突发读写调步',
    adaptiveBias: '自适应负载偏好',
    balancedProfile: '均衡 (高响应)',
    efficiencyProfile: '能效 (省电模式)',
    gamingProfile: '游戏 (超低抖动)',
    multitaskingProfile: '多任务 (充裕空间)',
    verifiedRecovery: '已验证恢复',
    activeMutations: '活动变异节点',
    journalSafe: '安全日志事务',
    refresh: '刷新',
    saving: '保存中...',
    saveArmed: '已实时生效于内核',
    language: '语言',
    theme: '主题',
    system: '跟随系统',
    light: '浅色',
    dark: '深色',
    reduceMotion: '减弱动态效果',
    oemBaseline: 'OEM 基线',
    engineActive: '引擎运行中',
    enginePaused: '引擎已暂停',
    quickSettings: '快捷磁贴设置',
    safeRecoveryTitle: '出厂基线守护者',
  },
  ru: {
    appTitle: 'CFAE System Framework',
    appSubtitle: 'v3.0.R · Автономная безопасность мутаций и баланс',
    stateBalanced: 'Сбалансированный',
    stateSustained: 'Длительная нагрузка',
    statePressure: 'Защита от давления',
    stateThermal: 'Термозащита',
    daemonRunning: 'Движок активен',
    daemonStandby: 'Движок в ожидании',
    startDaemon: 'Запустить фреймворк',
    stopDaemon: 'Остановить фреймворк',
    restoreBaseline: 'Восстановить заводской OEM',
    studioModalBtn: 'Архитектурная студия',
    thermalMatrixTitle: 'Матрица температур и PSI',
    autonomousTuningTitle: 'Автономная настройка и профили',
    longevityTitle: 'Долговечность оборудования',
    developerConsoleTitle: 'Консоль разработчика',
    liveStreamTitle: 'Поток реакций ядра в реальном времени',
    batteryStepCharging: 'Ступенчатая зарядка аккумулятора',
    storagePacing: 'Контроль всплесков ввода/вывода UFS',
    adaptiveBias: 'Адаптивный профиль нагрузки',
    balancedProfile: 'Баланс (Отзывчивый)',
    efficiencyProfile: 'Энергосбережение',
    gamingProfile: 'Игровой (Минимальный джиттер)',
    multitaskingProfile: 'Многозадачность (Запас)',
    verifiedRecovery: 'Проверенное восстановление',
    activeMutations: 'Активные мутации',
    journalSafe: 'Журнал безопасности',
    refresh: 'Обновить',
    saving: 'Сохранение...',
    saveArmed: 'Активировано в ядре',
    language: 'Язык',
    theme: 'Тема',
    system: 'Системная',
    light: 'Светлая',
    dark: 'Тёмная',
    reduceMotion: 'Уменьшить анимации',
    oemBaseline: 'OEM базис',
    engineActive: 'Движок активен',
    enginePaused: 'Движок на паузе',
    quickSettings: 'Быстрые настройки',
    safeRecoveryTitle: 'Страж базовых параметров',
  },
  pt: {
    appTitle: 'CFAE System Framework',
    appSubtitle: 'v3.0.R · Segurança de Mutação Autônoma e Equilíbrio',
    stateBalanced: 'Equilibrado',
    stateSustained: 'Carga Sustentada',
    statePressure: 'Guarda de Pressão',
    stateThermal: 'Guarda Térmica',
    daemonRunning: 'Motor Ativo',
    daemonStandby: 'Motor em Espera',
    startDaemon: 'Iniciar Framework',
    stopDaemon: 'Parar Framework',
    restoreBaseline: 'Restaurar Linha de Base OEM',
    studioModalBtn: 'Estúdio de Arquitetura',
    thermalMatrixTitle: 'Matriz Térmica e PSI',
    autonomousTuningTitle: 'Ajuste Autônomo e Perfis',
    longevityTitle: 'Longevidade de Hardware',
    developerConsoleTitle: 'Console do Desenvolvedor',
    liveStreamTitle: 'Transmissão em Tempo Real do Kernel',
    batteryStepCharging: 'Carga Escalonada da Bateria',
    storagePacing: 'Cadência de Rajada UFS',
    adaptiveBias: 'Perfil de Carga Adaptativo',
    balancedProfile: 'Equilibrado (Responsivo)',
    efficiencyProfile: 'Eficiência (Economia)',
    gamingProfile: 'Jogos (Baixo Jitter)',
    multitaskingProfile: 'Multitarefa (Margem)',
    verifiedRecovery: 'Recuperação Verificada',
    activeMutations: 'Mutações Ativas',
    journalSafe: 'Jornal de Segurança',
    refresh: 'Atualizar',
    saving: 'Salvando...',
    saveArmed: 'Armado em tempo real no kernel',
    language: 'Idioma',
    theme: 'Tema',
    system: 'Sistema',
    light: 'Claro',
    dark: 'Escuro',
    reduceMotion: 'Reduzir Animações',
    oemBaseline: 'Linha de base OEM',
    engineActive: 'Motor Ativo',
    enginePaused: 'Motor Pausado',
    quickSettings: 'Configurações Rápidas',
    safeRecoveryTitle: 'Guardião de Linha Base',
  },
  es: {
    appTitle: 'CFAE System Framework',
    appSubtitle: 'v3.0.R · Seguridad de Mutación Autónoma y Equilibrio',
    stateBalanced: 'Equilibrado',
    stateSustained: 'Carga Sostenida',
    statePressure: 'Protección de Presión',
    stateThermal: 'Protección Térmica',
    daemonRunning: 'Motor Activo',
    daemonStandby: 'Motor en Espera',
    startDaemon: 'Iniciar Framework',
    stopDaemon: 'Detener Framework',
    restoreBaseline: 'Restaurar Línea Base OEM',
    studioModalBtn: 'Estudio de Arquitectura',
    thermalMatrixTitle: 'Matriz Térmica y PSI',
    autonomousTuningTitle: 'Ajuste Autónomo y Perfiles',
    longevityTitle: 'Longevidad del Hardware',
    developerConsoleTitle: 'Consola de Desarrollador',
    liveStreamTitle: 'Flujo de Reacciones del Kernel en Vivo',
    batteryStepCharging: 'Carga Escalonada de Batería',
    storagePacing: 'Control de Ráfagas UFS',
    adaptiveBias: 'Perfil de Carga Adaptativo',
    balancedProfile: 'Equilibrado (Reactivo)',
    efficiencyProfile: 'Eficiencia (Ahorro de Batería)',
    gamingProfile: 'Juegos (Bajo Jitter)',
    multitaskingProfile: 'Multitarea (Margen)',
    verifiedRecovery: 'Recuperación Verificada',
    activeMutations: 'Mutaciones Activas',
    journalSafe: 'Diario de Seguridad',
    refresh: 'Actualizar',
    saving: 'Guardando...',
    saveArmed: 'Armado en tiempo real en kernel',
    language: 'Idioma',
    theme: 'Tema',
    system: 'Sistema',
    light: 'Claro',
    dark: 'Oscuro',
    reduceMotion: 'Reducir Animación',
    oemBaseline: 'Línea base OEM',
    engineActive: 'Motor Activo',
    enginePaused: 'Motor Pausado',
    quickSettings: 'Ajustes Rápidos',
    safeRecoveryTitle: 'Guardián de Línea Base',
  },
};

export function readLanguage(): SupportedLanguage {
  try {
    const saved = window.localStorage?.getItem(LANGUAGE_KEY) as SupportedLanguage | null;
    if (saved && ['en', 'id', 'zh', 'ru', 'pt', 'es'].includes(saved)) {
      return saved;
    }
    const nav = (navigator.language || '').toLowerCase();
    if (nav.startsWith('id')) return 'id';
    if (nav.startsWith('zh')) return 'zh';
    if (nav.startsWith('ru')) return 'ru';
    if (nav.startsWith('pt')) return 'pt';
    if (nav.startsWith('es')) return 'es';
    return 'en';
  } catch {
    return 'en';
  }
}

export function saveLanguage(lang: SupportedLanguage): void {
  try {
    window.localStorage?.setItem(LANGUAGE_KEY, lang);
  } catch {}
  window.dispatchEvent(new CustomEvent('coreflow-language', { detail: lang }));
}

export function translate(key: string, lang: SupportedLanguage): string {
  const table = TRANSLATIONS[lang] || TRANSLATIONS.en;
  return table[key] || TRANSLATIONS.en[key] || key;
}

export function readAppearance(): Appearance {
  try {
    const value = JSON.parse(window.localStorage?.getItem(APPEARANCE_KEY) || '{}');
    return {
      theme: ['system', 'light', 'dark'].includes(value.theme) ? value.theme : 'system',
      reduceMotion: value.reduceMotion === true,
      language: readLanguage(),
    };
  } catch {
    return { theme: 'system', reduceMotion: false, language: 'en' };
  }
}

export function applyAppearance(value: Appearance): void {
  const dark = value.theme === 'dark' || (value.theme === 'system' && window.matchMedia?.('(prefers-color-scheme: dark)').matches);
  document.documentElement.dataset.theme = dark ? 'dark' : 'light';
  document.documentElement.dataset.motion = value.reduceMotion ? 'reduce' : 'system';
}

export function saveAppearance(value: Appearance): boolean {
  applyAppearance(value);
  let stored = true;
  try {
    window.localStorage.setItem(APPEARANCE_KEY, JSON.stringify(value));
  } catch {
    stored = false;
  }
  if (value.language) {
    saveLanguage(value.language);
  }
  window.dispatchEvent(new CustomEvent('coreflow-appearance', { detail: value }));
  return stored;
}
