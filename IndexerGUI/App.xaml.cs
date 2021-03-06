﻿using System;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Threading;
using System.Windows;
using System.Windows.Markup;
using CLIInterop;

namespace Indexer
{
    public partial class App : Application
    {
        private static EventWaitHandle eventWaitHandle; // Used implicitly.
        private static string namePrefix;

        [STAThread]
        private void App_Startup(object sender, StartupEventArgs e)
        {
            var exeDir = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            Directory.SetCurrentDirectory(exeDir);

            Log.Instance.Debug("App_Startup called. Starting application.");

            AppDomain.CurrentDomain.UnhandledException += AppDomainOnUnhandledException;
            CmdArgumentsParser.Init(e);

            try
            {
                // Ensure the current culture passed into bindings is the OS culture.
                // By default, WPF uses en-US as the culture, regardless of the system settings.
                FrameworkElement.LanguageProperty.OverrideMetadata(
                    typeof (FrameworkElement),
                    new FrameworkPropertyMetadata(
                        XmlLanguage.GetLanguage(CultureInfo.CurrentCulture.IetfLanguageTag)));
            }
            catch (Exception ex)
            {
                Log.Instance.Error("LanguageProperty.OverrideMetadata throw exception: " + ex.Message);
            }

            if (SystemConfigFlagsWrapper.Instance().PipeManager)
            {
                ShutdownMode = ShutdownMode.OnExplicitShutdown;
                CreateAndStartPipeManager(e);
            }
            else
            {
                ShutdownMode = ShutdownMode.OnMainWindowClose;
            }

            try
            {
                var startInfo = new System.Diagnostics.ProcessStartInfo("GUP.exe")
                {
                    WorkingDirectory = "updater"
                };

                System.Diagnostics.Process.Start(startInfo);
            }
            catch (Exception ex)
            {
                Log.Instance.Error("Checking for update failed: " + ex.Message);
            }

            if (AppAlreadyRuns) return;

            if (SystemConfigFlagsWrapper.Instance().TrayIcon)
            {
                TrayIconManager.CreateIcon();
            }

            if (SystemConfigFlagsWrapper.Instance().ShowDebugLogWindow)
            {
                var wnd = new DebugLogWindow();
                wnd.Show();
            }

            var visibility = Visibility.Visible;
            if (e.Args.Length > 0 && e.Args[0] == "-scheduled")
                visibility = Visibility.Collapsed;

            Helper.OpenNewIndexerWnd(visibility);
        }

        private static void CreateAndStartPipeManager(StartupEventArgs e)
        {
            Log.Instance.Debug("Creating NamedPipeManager.");

            namePrefix = Helper.GetNamePrefix();
            var pipeManager = new NamedPipeManager(Current.Dispatcher);

            Log.Instance.Debug("Checking if app already runs.");

            if (AppAlreadyRuns)
            {
                var initialDir = CmdArgumentsParser.FilterDirPath;
                Log.Instance.Debug("App already running, starting client. Filter dir path = " + initialDir);
                pipeManager.StartClient(initialDir);
                Current.Shutdown();
            }
            else
            {
                Log.Instance.Debug("App not running, starting server.");
                pipeManager.StartServer();
            }
        }

        private static void AppDomainOnUnhandledException(object sender,
            UnhandledExceptionEventArgs unhandledExceptionEventArgs)
        {
            var e = (Exception) unhandledExceptionEventArgs.ExceptionObject;
            Log.Instance.Error("AppDomainOnUnhandledException: " + e.Message + "Stack trace: " + e.StackTrace);
        }

        protected override void OnExit(ExitEventArgs e)
        {
            TrayIconManager.Dispose();
            base.OnExit(e);
        }


        private static bool? appAlreadyRuns;
        private static bool AppAlreadyRuns
        {
            get
            {
                if (appAlreadyRuns == null)
                {
                    bool created;
                    eventWaitHandle = new EventWaitHandle(false, EventResetMode.ManualReset, namePrefix, out created);
                    appAlreadyRuns = !created;
                }
                return appAlreadyRuns.Value;
            }
        }
    }
}